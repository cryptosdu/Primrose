#pragma once

#include <vector>

#include "fhe_adapter.h"
#include "params.h"

class SimilarityCalculator {

protected:
    const fhe::CKKSEncoder &encoder_;
    const fhe::Evaluator &evaluator_;

    std::vector<fhe::Plaintext> tmp_plaintexts_;
    fhe::Ciphertext tmp_ciphertext_;

public:
    SimilarityCalculator(const fhe::CKKSEncoder &encoder,
                         const fhe::Evaluator &evaluator) : encoder_(encoder),
                                                             evaluator_(evaluator),
                                                             tmp_plaintexts_(Params::dimension) {
    }

    virtual ~SimilarityCalculator() = default;

    virtual std::vector<std::vector<fhe::Ciphertext>> compute(const std::vector<fhe::Ciphertext> &queries,
                                                               const std::vector<float> &db,
                                                               size_t dimension,
                                                               double scale) {
        return {};
    }
};

class InnerProductCalculator : public SimilarityCalculator {

public:
    InnerProductCalculator(const fhe::CKKSEncoder &encoder,
                           const fhe::Evaluator &evaluator) : SimilarityCalculator(encoder, evaluator) {

    }

    std::vector<std::vector<fhe::Ciphertext>> compute(const std::vector<fhe::Ciphertext> &queries,
                                                       const std::vector<float> &db, size_t dimension,
                                                       double scale) override;
};

class EuclideanCalculator : public SimilarityCalculator {

public:
    EuclideanCalculator(const fhe::CKKSEncoder &encoder,
                        const fhe::Evaluator &evaluator) : SimilarityCalculator(encoder, evaluator) {

    }

    std::vector<std::vector<fhe::Ciphertext>> compute(const std::vector<fhe::Ciphertext> &queries,
                                                       const std::vector<float> &db, size_t dimension,
                                                       double scale) override;

};