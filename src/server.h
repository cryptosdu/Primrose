#pragma once

#include "params.h"
#include "calculator.h"
#include "util/vectorutil.h"
#include "models.h"
#include "fhe_adapter.h"

class Server {

public:

    Server(fhe::Context &context, size_t cluster_num) : encoder_(context),
                                                        evaluator_(context),
                                                        tmp_plaintexts_(Params::dimension),
                                                        cluster_num_(cluster_num) {
        calculators_.reserve(Params::response_thread_num);
        for (int i = 0; i < Params::response_thread_num; ++i) {
            calculators_.push_back(std::move(create_calculator()));
        }
    }

    void read_data(const std::string &path) {
        db_data_ = util::read_npy_data(path);
    }

    void generate_random_data(size_t size) {
        util::generate_random_vectors(size, Params::dimension, db_data_);
    }

    const std::vector<float> &get_centroids() {
        return centroids_;
    }

    void convert_data_to_clusters();

    Response generate_response(const Query &query);

private:

    std::unique_ptr<SimilarityCalculator> create_calculator() {
        switch (Params::similarity_type) {
            case SimilarityType::inner_product:
                return std::make_unique<InnerProductCalculator>(encoder_, evaluator_);
            case SimilarityType::euclidean:
                return std::make_unique<EuclideanCalculator>(encoder_, evaluator_);
            default:
                throw std::invalid_argument("Unknown similarity type");
        }
    }

    const fhe::CKKSEncoder encoder_;
    const fhe::Evaluator evaluator_;

    std::vector<float> db_data_;
    std::vector<std::vector<float>> clusters_;
    std::vector<std::vector<int64_t>> indexes_;
    std::vector<float> centroids_;

    std::vector<fhe::Plaintext> tmp_plaintexts_;
    fhe::Ciphertext tmp_ciphertext_;

    std::vector<std::unique_ptr<SimilarityCalculator>> calculators_;

    size_t cluster_num_;
};