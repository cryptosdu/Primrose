#pragma once

#include "models.h"
#include "util/vectorutil.h"
#include "params.h"

class Client {

    std::vector<fhe::CKKSEncoder> encoders_;
    const fhe::KeyGenerator keygen_;
    const fhe::SecretKey secret_key_;
    const fhe::Encryptor encryptor_;
    fhe::Decryptor decryptor_;
    std::vector<float> query_data_;
    size_t cluster_num_;

public:

    Client(fhe::Context &context, size_t cluster_num) : keygen_(context),
                                                        secret_key_(keygen_.secret_key()),
                                                        encryptor_(context, secret_key_),
                                                        decryptor_(context, secret_key_),
                                                        cluster_num_(cluster_num) {
        encoders_.reserve(Params::query_thread_num);
        for (int i = 0; i < Params::query_thread_num; ++i) {
            encoders_.emplace_back(context);
        }
    }

    void read_data(const std::string &path) {
        query_data_ = util::read_npy_data(path);
    }

    void generate_random_data(size_t size) {
        util::generate_random_vectors(size, Params::dimension, query_data_);
    }

    Query generate_query(const std::vector<float> &centroids);

    std::vector<std::tuple<size_t, double>> decode_response(const Query &query, const Response &response, fhe::Context &context);

};