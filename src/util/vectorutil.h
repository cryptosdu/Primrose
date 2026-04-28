#pragma once

#include <string>
#include <vector>

#include "../fhe_adapter.h"
#include "taskmanager.h"

namespace util {

    std::vector<float> read_npy_data(const std::string &npy_name, size_t *dimension = nullptr);

    void kmeans_clustering(
            size_t dimension,
            std::vector<float> &training_set,
            std::vector<std::vector<float>> &cluster,
            std::vector<std::vector<int64_t>> &indexes,
            std::vector<float> &centroids,
            size_t centroids_size);

    std::vector<float> format_matrix(float *data, size_t data_length, size_t slots, size_t dimension, float scale);

    std::vector<size_t>
    nearest_centroid(const std::vector<float> &data, const std::vector<float> &centroid, size_t dimension);

    std::vector<size_t> nearest(float *data, size_t data_size, float *db, size_t db_size, size_t dimension);

    void print_vec(std::vector<double> &vec);

    void print_ctx(fhe::Ciphertext &ctx, fhe::CKKSEncoder &encoder, fhe::Decryptor &decryptor);

    void save_data_to_csv(const std::vector<std::tuple<size_t, double>> &data,
                          const std::string &filename,
                          bool include_header = true);

    void generate_random_vectors(size_t size, size_t dimension, std::vector<float> &vectors);

} // util
