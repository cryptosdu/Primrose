
#include <cmath>
#include <iostream>
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>

#include "server.h"
#include "util/taskmanager.h"

void Server::convert_data_to_clusters() {
    size_t cluster_num = cluster_num_;
    size_t dimension = Params::dimension;
    clusters_.resize(cluster_num);
    indexes_.resize(cluster_num);
    centroids_.resize(cluster_num * dimension);
    util::kmeans_clustering(dimension, db_data_, clusters_, indexes_, centroids_, cluster_num);
    db_data_.clear();
}

Response Server::generate_response(const Query &query) {
    size_t cluster_num = cluster_num_;
    size_t dimension = Params::dimension;
    size_t scale_power = Params::scale_power;
    double scale = pow(2.0, scale_power);

    const std::vector<std::vector<fhe::Ciphertext>> &query_cts = query.get_query_cts();
    Response response;
    std::vector<std::vector<std::vector<fhe::Ciphertext>>> &response_cts = response.get_response_cts();
    response_cts.resize(cluster_num);

    static size_t response_counter = 0;
    std::stringstream cache_dir_ss;
    cache_dir_ss << "/tmp/primrose_response_" << getpid() << "_" << response_counter++;
    std::string cache_dir = cache_dir_ss.str();
    mkdir(cache_dir.c_str(), 0755);

    response.set_cache_dir(cache_dir);
    response.enable_cache(true);
    response.set_cluster_num(cluster_num);

    auto procFunc = [&](size_t idx, size_t thread_idx) {
        const std::vector<fhe::Ciphertext> &queries = query_cts[idx];
        std::vector<float> &cluster = clusters_[idx];
        std::cout << "Thread " << thread_idx << " is calculating cluster " << idx + 1 << "/" << cluster_num << std::endl;
        response_cts[idx] = calculators_[thread_idx]->compute(queries, cluster, dimension, scale);
#ifdef PRIMROSE_ENABLE_GPU
        fhe::synchronizeCurrentStream();
#endif
    };
    auto consFunc = [&](size_t idx) {
        std::cout << "Cluster " << idx + 1 << " calculation is finished." << std::endl;
        size_t total_size = 0;

        for (size_t vec_idx = 0; vec_idx < response_cts[idx].size(); ++vec_idx) {
            auto &vector_cts = response_cts[idx][vec_idx];
#ifndef PRIMROSE_ENABLE_GPU
            for (auto &ct : vector_cts) {
                total_size += ct.save_size();
            }
#endif
            response.save_ct_to_disk(idx, vec_idx, vector_cts);
            vector_cts.clear();
        }
        response.get_cts_size() += total_size;
        response_cts[idx].clear();
    };
    TaskManager taskManager(cluster_num, Params::response_thread_num, procFunc, consFunc);

    taskManager.start();
    taskManager.join();


    std::cout << std::endl;
    response.get_indexes() = indexes_;
    response.clear_memory_cts();
    return response;
}