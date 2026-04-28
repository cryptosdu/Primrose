#pragma once

#include <cinttypes>
#include <cstddef>
#include <array>

enum class SimilarityType : uint8_t {
    inner_product = 1,
    euclidean = 2,
    hamming = 3
};

namespace Params {
    inline static constexpr size_t poly_modulus_degree = 4096;
    inline static constexpr size_t scale_power = 25;
#ifdef PRIMROSE_ENABLE_GPU
    inline static constexpr std::array<int, 2> modulus_bit_sizes = {60, 60};
#else
    inline static constexpr std::array<int, 1> modulus_bit_sizes = {60};
#endif
    inline static constexpr SimilarityType similarity_type = SimilarityType::inner_product;
    inline static constexpr size_t dimension = 192;

    inline static constexpr size_t data_gen_thread_num = 24;
    inline static constexpr size_t query_thread_num = 16;
    inline static constexpr size_t response_thread_num = 1;

};
