#pragma once

#include <condition_variable>
#include <queue>
#include <functional>
#include <stdexcept>
#include <thread>

template<typename T, size_t max_size = 100>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() : closed_(false) {}

    ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

    template<typename U>
    void push(U &&new_value) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            push_cond_.wait(lock, [this] { return this->queue_.size() < max_size; });
            queue_.push(std::forward<U>(new_value));
        }
        pop_cond_.notify_one();
    }

    bool waitPop(T &value) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            pop_cond_.wait(lock, [this] { return closed_ || !queue_.empty(); });
            if (queue_.empty()) return false;
            value = std::move(queue_.front());
            queue_.pop();
        }
        push_cond_.notify_one();
        return true;
    }

    bool pop(T &value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        pop_cond_.notify_all();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    mutable std::condition_variable push_cond_;
    mutable std::condition_variable pop_cond_;
    bool closed_;
};

class Processor {
public:
    using ProcessFunc = std::function<void(size_t, size_t)>;

    Processor(ProcessFunc f, ThreadSafeQueue<size_t> *inQ, ThreadSafeQueue<size_t> *outQ, size_t index)
            : processFunc_(std::move(f)), inQueue_(inQ), outQueue_(outQ), index_(index) {}

    void run() {
        if (!inQueue_) {
            throw std::runtime_error("inQueue is null");
        }
        size_t idx;
        while (inQueue_->waitPop(idx)) {
            processFunc_(idx, index_);
            if (outQueue_) {
                outQueue_->push(idx);
            }
        }
    }

private:
    ProcessFunc processFunc_;
    ThreadSafeQueue<size_t> *inQueue_;
    ThreadSafeQueue<size_t> *outQueue_;
    size_t index_;
};

class Consumer {
public:
    using ConsumeFunc = std::function<void(size_t)>;

    Consumer() = default;
    Consumer(ConsumeFunc f, ThreadSafeQueue<size_t> *inQ) : consumeFunc_(std::move(f)), queue_(inQ) {}

    void run() {
        size_t item;
        while (queue_->waitPop(item)) {
            consumeFunc_(item);
        }
    }

private:
    ConsumeFunc consumeFunc_ = nullptr;
    ThreadSafeQueue<size_t> *queue_ = nullptr;
};

class TaskManager {
public:
    using ProcessFunc = typename Processor::ProcessFunc;
    using ConsumeFunc = typename Consumer::ConsumeFunc;

    TaskManager(size_t parallel_size,
                size_t thread_num,
                const ProcessFunc &f2,
                const ConsumeFunc &f3 = nullptr) : parallel_size_(parallel_size),
                                                   consumeFunc_(f3) {
        for (size_t i = 0; i < thread_num; ++i) {
            processors_.emplace_back(f2, &queue1_, &queue2_, i);
        }
        if (f3) {
            consumer_ = Consumer(f3, &queue2_);
        }
    }

    void start() {
        producerThread_ = std::thread(&TaskManager::produce_index, this);

        processorThreads_.clear();
        for (auto &processor: processors_) {
            processorThreads_.emplace_back(&Processor::run, &processor);
        }

        if (consumeFunc_) {
            consumerThread_ = std::thread(&Consumer::run, &consumer_);
        }
    }

    void produce_index() {
        for (int i = 0; i < parallel_size_; ++i) {
            queue1_.push(i);
        }
        queue1_.close();
    }

    void join() {
        if (producerThread_.joinable()) {
            producerThread_.join();
        }

        queue1_.close();

        for (auto &t: processorThreads_) {
            if (t.joinable()) {
                t.join();
            }
        }

        queue2_.close();

        if (consumeFunc_ && consumerThread_.joinable()) {
            consumerThread_.join();
        }

        processorThreads_.clear();
    }

private:
    ThreadSafeQueue<size_t> queue1_;
    ThreadSafeQueue<size_t> queue2_;

    std::vector<Processor> processors_;
    Consumer consumer_;

    std::thread producerThread_;
    std::vector<std::thread> processorThreads_;
    std::thread consumerThread_;

    size_t parallel_size_;

    ConsumeFunc consumeFunc_ = nullptr;
};


