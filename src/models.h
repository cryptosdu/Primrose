#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "fhe_adapter.h"

class Query {
    size_t query_size_ = 0;
    std::vector<std::vector<fhe::Ciphertext>> query_cts_;
    std::vector<std::vector<size_t>> nearest_idx_revert_;
public:
    Query(size_t cluster_num) : query_cts_(cluster_num), nearest_idx_revert_(cluster_num) {}

    Query(const Query &) = default;

    Query(Query &&) = default;

    ~Query() = default;

    Query &operator=(const Query &) = default;

    Query &operator=(Query &&) = default;

    size_t &get_query_size() {
        return query_size_;
    }

    const size_t &get_query_size() const {
        return query_size_;
    }

    std::vector<std::vector<fhe::Ciphertext>> &get_query_cts() {
        return query_cts_;
    }

    const std::vector<std::vector<fhe::Ciphertext>> &get_query_cts() const {
        return query_cts_;
    }

    std::vector<std::vector<size_t>> &get_nearest_idx_revert() {
        return nearest_idx_revert_;
    }

    const std::vector<std::vector<size_t>> &get_nearest_idx_revert() const {
        return nearest_idx_revert_;
    }

    size_t calculate_total_size() {
        size_t total_size = 0;
#ifndef PRIMROSE_ENABLE_GPU
        for (auto &vector_cts : query_cts_) {
            for (auto &ct : vector_cts) {
                total_size += ct.save_size();
            }
        }
#endif
        for (auto &vector_idx : nearest_idx_revert_) {
            total_size += (vector_idx.size() * sizeof(size_t));
        }
        return total_size;
    }

    void clear() {
        for (auto &vec : query_cts_) {
            vec.clear();
        }
        query_cts_.clear();
        nearest_idx_revert_.clear();
    }
};

class Response {
    std::vector<std::vector<std::vector<fhe::Ciphertext>>> response_cts_;
    std::vector<std::vector<int64_t>> indexes_;
    std::string cache_dir_;
    bool use_cache_ = false;
    size_t cluster_num_ = 0;
    size_t cts_size_ = 0;
public:
    Response() = default;

    Response(const Response &) = default;

    Response(Response &&) = default;

    ~Response() = default;

    Response &operator=(const Response &) = default;

    Response &operator=(Response &&) = default;

    std::vector<std::vector<std::vector<fhe::Ciphertext>>> &get_response_cts() {
        return response_cts_;
    }

    const std::vector<std::vector<std::vector<fhe::Ciphertext>>> &get_response_cts() const {
        return response_cts_;
    }

    std::vector<std::vector<int64_t>> &get_indexes() {
        return indexes_;
    }

    const std::vector<std::vector<int64_t>> &get_indexes() const {
        return indexes_;
    }

    size_t &get_cts_size() {
        return cts_size_;
    }

    void set_cache_dir(const std::string &cache_dir) {
        cache_dir_ = cache_dir;
    }

    const std::string &get_cache_dir() const {
        return cache_dir_;
    }

    bool use_cache() const {
        return use_cache_;
    }

    void enable_cache(bool use_cache) {
        use_cache_ = use_cache;
    }

    void set_cluster_num(size_t cluster_num) {
        cluster_num_ = cluster_num;
    }

    void save_ct_to_disk(size_t cluster_idx, size_t vec_idx, const std::vector<fhe::Ciphertext> &cts) {
        if (!use_cache_ || cache_dir_.empty()) return;

        std::string filename = cache_dir_ + "/cluster_" + std::to_string(cluster_idx) +
                               "_vec_" + std::to_string(vec_idx) + ".bin";
        std::ofstream out(filename, std::ios::binary);
        if (!out) return;

        size_t ct_count = cts.size();
        out.write(reinterpret_cast<const char*>(&ct_count), sizeof(size_t));

        for (const auto &ct : cts) {
            ct.save(out);
        }
        out.close();
    }

    void load_ct_from_disk(size_t cluster_idx, size_t vec_idx, std::vector<fhe::Ciphertext> &cts, fhe::Context &context) const {
        if (!use_cache_ || cache_dir_.empty()) return;

        std::string filename = cache_dir_ + "/cluster_" + std::to_string(cluster_idx) +
                               "_vec_" + std::to_string(vec_idx) + ".bin";
        std::ifstream in(filename, std::ios::binary);
        if (!in) return;

        size_t ct_count = 0;
        in.read(reinterpret_cast<char*>(&ct_count), sizeof(size_t));
        cts.resize(ct_count);

        for (size_t i = 0; i < ct_count; ++i) {
            cts[i].load(context, in);
        }
        in.close();
    }

    void clear_memory_cts() {
        for (auto &vec : response_cts_) {
            for (auto &inner : vec) {
                inner.clear();
            }
            vec.clear();
        }
        response_cts_.clear();
    }

    size_t calculate_total_size() {
        size_t total_size = cts_size_;

        for (auto &vector_idx : indexes_) {
            total_size += (vector_idx.size() * sizeof(int64_t));
        }
        return total_size;
    }
};

