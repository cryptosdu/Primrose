
#include <iostream>

#include "calculator.h"

std::vector<std::vector<fhe::Ciphertext>> InnerProductCalculator::compute(const std::vector<fhe::Ciphertext> &queries,
                                                                           const std::vector<float> &db,
                                                                           size_t dimension,
                                                                           double scale) {
    size_t db_num = db.size() / dimension;
    size_t query_size = queries.size() / dimension;
    std::vector<std::vector<fhe::Ciphertext>> responses(db_num);
    for (int j = 0; j < db_num; ++j) {
        const float *vec_ptr = db.data() + j * dimension;
        std::vector<fhe::Ciphertext> &resp = responses[j];
        resp.resize(query_size);
        for (int k = 0; k < dimension; ++k) {
            encoder_.encode(vec_ptr[k], scale, tmp_plaintexts_[k]);
        }
        for (int k = 0; k < query_size; ++k) {
            auto queries_ptr = queries.data() + k * dimension;
            evaluator_.multiply_plain(queries_ptr[0], tmp_plaintexts_[0], resp[k]);
            for (int l = 1; l < dimension; ++l) {
                evaluator_.multiply_plain(queries_ptr[l], tmp_plaintexts_[l], tmp_ciphertext_);
                evaluator_.add_inplace(resp[k], tmp_ciphertext_);
            }
        }
    }

    return responses;
}

std::vector<std::vector<fhe::Ciphertext>> EuclideanCalculator::compute(const std::vector<fhe::Ciphertext> &queries,
                                                                        const std::vector<float> &db,
                                                                        size_t dimension,
                                                                        double scale) {
    size_t db_num = db.size() / dimension;
    size_t query_size = queries.size() / dimension;
    std::vector<std::vector<fhe::Ciphertext>> responses(db_num);
    for (int j = 0; j < db_num; ++j) {
        const float *vec_ptr = db.data() + j * dimension;
        std::vector<fhe::Ciphertext> &resp = responses[j];
        resp.reserve(query_size);
        for (int k = 0; k < dimension; ++k) {
            encoder_.encode(vec_ptr[k], scale, tmp_plaintexts_[k]);
        }
        for (int k = 0; k < query_size; ++k) {
            auto queries_ptr = queries.data() + k * dimension;
            evaluator_.sub_plain(queries_ptr[0], tmp_plaintexts_[0], resp[k]);
            evaluator_.square_inplace(resp[k]);
            for (int l = 1; l < dimension; ++l) {
                evaluator_.sub_plain(queries_ptr[l], tmp_plaintexts_[l], tmp_ciphertext_);
                evaluator_.square_inplace(tmp_ciphertext_);
                evaluator_.add_inplace(resp[k], tmp_ciphertext_);
            }
        }
    }

    return responses;
}

