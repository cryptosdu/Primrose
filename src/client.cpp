#include <cfloat>
#include <cmath>

#include "client.h"
#include "params.h"
#include "util/vectorutil.h"

Query Client::generate_query(const std::vector<float> &centroids) {
    size_t cluster_num = cluster_num_;
    size_t dimension = Params::dimension;
    size_t query_size = query_data_.size() / dimension;
    size_t scale_power = Params::scale_power;
    double scale = pow(2.0, scale_power);
    size_t slots = Params::poly_modulus_degree / 2;
    std::vector<size_t> nearest_idx = util::nearest_centroid(query_data_, centroids, dimension);

    Query query(cluster_num);
    query.get_query_size() = query_size;
    auto &nearest_idx_revert = query.get_nearest_idx_revert();
    std::vector<std::vector<float>> divided_data(cluster_num);
    for (auto data: divided_data) {
        data.reserve(query_size / cluster_num);
    }
    for (int i = 0; i < query_size; ++i) {
        auto idx = nearest_idx[i];
        nearest_idx_revert[idx].push_back(i);
        auto ptr = query_data_.data() + i * dimension;
        divided_data[idx].insert(divided_data[idx].end(), ptr, ptr + dimension);
    }
    query_data_.clear();
    nearest_idx.clear();

    std::vector<std::vector<float>> formated_data(cluster_num);
    auto procFunc = [&](size_t idx, size_t thread_idx) {
        formated_data[idx] = util::format_matrix(divided_data[idx].data(), divided_data[idx].size(), slots, dimension,1);
    };
    auto consFunc = [&](size_t idx) {
    };
    TaskManager taskManager1(cluster_num, Params::query_thread_num, procFunc, consFunc);

    taskManager1.start();
    taskManager1.join();

    divided_data.clear();


    std::vector<fhe::Plaintext> plaintexts(cluster_num);
    std::vector<std::vector<fhe::Ciphertext>> &query_cts = query.get_query_cts();

    auto procFunc2 = [&](size_t idx, size_t thread_idx) {
        auto &data = formated_data[idx];
        auto num = formated_data[idx].size() / slots;
        query_cts[idx].resize(num);
        for (int j = 0; j < num; ++j) {
            encoders_[thread_idx].encode(data.data() + j * slots, slots, scale, plaintexts[idx]);
            encryptor_.encrypt_symmetric(plaintexts[idx], query_cts[idx][j]);
#ifdef PRIMROSE_ENABLE_GPU
            fhe::synchronizeCurrentStream();
#endif
        }
    };
    auto consFunc2 = [&](size_t idx) {
    };
    TaskManager taskManager2(cluster_num, Params::query_thread_num, procFunc2, consFunc2);

    taskManager2.start();
    taskManager2.join();

    return query;
}

std::vector<std::tuple<size_t, double>> Client::decode_response(const Query &query, const Response &response, fhe::Context &context) {
    size_t cluster_num = cluster_num_;

    size_t query_size = query.get_query_size();
    auto &response_cts = response.get_response_cts();
    auto &indexes = response.get_indexes();
    auto &nearest_idx_revert = query.get_nearest_idx_revert();

    fhe::Plaintext ptx;
    std::vector<double> tmp_vector;
    std::vector<std::tuple<size_t, double>> max_results(query_size, std::tuple{0, DBL_MIN});
    for (int i = 0; i < cluster_num; ++i) {
        const std::vector<std::vector<fhe::Ciphertext>> &responses = response_cts[i];
        const std::vector<int64_t> &index = indexes[i];
        const std::vector<size_t> &idx_revert = nearest_idx_revert[i];
        size_t vec_count = responses.size();

        if (vec_count == 0) {
            vec_count = indexes[i].size();
        }

        for (int j = 0; j < vec_count; ++j) {
            std::vector<fhe::Ciphertext> resp;
            response.load_ct_from_disk(i, j, resp, context);
            std::vector<double> result;
            for (int k = 0; k < resp.size(); ++k) {
                const fhe::Ciphertext &ctx = resp[k];
                decryptor_.decrypt(ctx, ptx);
                encoders_[0].decode(ptx, tmp_vector);
                result.insert(result.end(), tmp_vector.begin(), tmp_vector.end());
            }
            resp.clear();

            for (int k = 0; k < idx_revert.size(); ++k) {
                auto idx = idx_revert[k];
                auto similarity = result[k];
                auto &[max_idx, max_similarity] = max_results[idx];
                if (similarity > max_similarity) {
                    max_idx = index[j];
                    max_similarity = similarity;
                }
            }
            result.clear();
        }
    }
    return max_results;
}
