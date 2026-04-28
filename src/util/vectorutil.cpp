
#include "vectorutil.h"
#include <cnpy.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <iomanip>
#include <random>

#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFFlat.h>
#include <faiss/invlists/InvertedLists.h>
#include <faiss/Clustering.h>

#include "../params.h"

namespace util {

    std::vector<float> read_npy_data(const std::string &npy_name, size_t *dimension) {
        cnpy::NpyArray arr = cnpy::npy_load(npy_name);
        std::vector<size_t> shape = arr.shape;
        auto size = shape[0];
        auto dim = shape[1];
        auto *data = arr.data<float>();
        std::vector<float> result(size * dim);
        memcpy(result.data(), data, sizeof(float) * size * dim);
        if (dimension) {
            *dimension = dim;
        }
        return result;
    }

    void kmeans_clustering(
            size_t dimension,
            std::vector<float> &training_set,
            std::vector<std::vector<float>> &cluster,
            std::vector<std::vector<int64_t>> &indexes,
            std::vector<float> &centroids,
            size_t centroids_size) {
        auto training_set_size = training_set.size() / dimension;
        faiss::Clustering clus(dimension, centroids_size);
        std::unique_ptr<faiss::IndexFlat> quantizer_ptr;
        if (Params::similarity_type == SimilarityType::inner_product) {
            quantizer_ptr = std::make_unique<faiss::IndexFlatIP>(dimension);
        } else if (Params::similarity_type == SimilarityType::euclidean) {
            quantizer_ptr = std::make_unique<faiss::IndexFlatL2>(dimension);
        }
        clus.train(std::min(training_set_size, (size_t) 100000), training_set.data(), *quantizer_ptr);
        memcpy(centroids.data(), clus.centroids.data(), sizeof(float) * dimension * centroids_size);

        faiss::IndexIVFFlat index(quantizer_ptr.get(), dimension, centroids_size);
        index.add(training_set_size, training_set.data());
        std::vector<float>().swap(training_set); // to clear the memory used by training_set
        auto inv_lists = dynamic_cast<faiss::ArrayInvertedLists *>(index.invlists);
        for (int i = 0; i < centroids_size; i++) {
            auto &inv_data = inv_lists->codes[i];
            auto &clu_data = cluster[i];
            auto inv_len = sizeof(unsigned char) * inv_data.size();
            clu_data.resize(inv_len / sizeof(float));
            memcpy(clu_data.data(), inv_data.data(), inv_len);

            indexes[i] = inv_lists->ids[i];
        }
    }

    std::vector<float> format_matrix(float *data, size_t data_length, size_t slots, size_t dimension, float scale) {
        auto vec_num = data_length / dimension;
        auto full_ptx_num = vec_num / slots * dimension;
        auto ptx_num = full_ptx_num + (vec_num % slots == 0 ? 0 : dimension);
        std::vector<float> matrix(ptx_num * slots);
        for (size_t offset = 0; offset < full_ptx_num; offset += dimension) {
            auto matrix_ptr = matrix.data() + offset * slots;
            auto data_ptr = data + offset * slots;
            for (int i = 0; i < slots; ++i) {
                auto column_ptr = matrix_ptr + i;
                auto row_ptr = data_ptr + i * dimension;
                for (int j = 0; j < dimension; ++j) {
                    *(column_ptr + j * slots) = *(row_ptr + j);
                }
            }
        }
        if (full_ptx_num != ptx_num) {
            auto matrix_ptr = matrix.data() + full_ptx_num * slots;
            std::fill_n(matrix_ptr, dimension * slots, 0);
            auto data_ptr = data + full_ptx_num * slots;
            auto num = vec_num % slots;
            for (int i = 0; i < num; ++i) {
                auto column_ptr = matrix_ptr + i;
                auto row_ptr = data_ptr + i * dimension;
                for (int j = 0; j < dimension; ++j) {
                    *(column_ptr + j * slots) = *(row_ptr + j);
                }
            }
        }
        return matrix;
    }

    std::vector<size_t>
    nearest_centroid(const std::vector<float> &data, const std::vector<float> &centroid, size_t dimension) {
        size_t data_size = data.size() / dimension;
        size_t centroid_size = centroid.size() / dimension;
        std::unique_ptr<faiss::IndexFlat> index;
        if (Params::similarity_type == SimilarityType::inner_product) {
            index = std::make_unique<faiss::IndexFlatIP>(dimension);
        } else if (Params::similarity_type == SimilarityType::euclidean) {
            index = std::make_unique<faiss::IndexFlatL2>(dimension);
        }
        index->add(centroid_size, centroid.data());
        assert(index->is_trained);
        auto distance = std::make_unique<float[]>(data_size);
        std::vector<size_t> idx(data_size);
        index->search(data_size, data.data(), 1, distance.get(), reinterpret_cast<faiss::idx_t *>(idx.data()));
        return idx;
    }

    std::vector<size_t> nearest(float *data, size_t data_size, float *db, size_t db_size, size_t dimension) {
        std::unique_ptr<faiss::IndexFlat> index;
        if (Params::similarity_type == SimilarityType::inner_product) {
            index = std::make_unique<faiss::IndexFlatIP>(dimension);
        } else if (Params::similarity_type == SimilarityType::euclidean) {
            index = std::make_unique<faiss::IndexFlatL2>(dimension);
        }
        index->add(db_size, db);
        assert(index->is_trained);
        auto distance = std::make_unique<float[]>(data_size * 10);
        std::vector<size_t> idx(data_size * 10);
        index->search(data_size, data, 10, distance.get(), reinterpret_cast<faiss::idx_t *>(idx.data()));
        return idx;
    }

    void print_vec(std::vector<double> &vec) {
        std::cout << "Ciphertext = [";
        for (double v: vec) {
            std::cout << v << ", ";
        }
        std::cout << "]" << std::endl;
    }

    void print_ctx(fhe::Ciphertext &ctx, fhe::CKKSEncoder &encoder, fhe::Decryptor &decryptor) {
        fhe::Plaintext ptx;
        decryptor.decrypt(ctx, ptx);
        std::vector<double> vec;
        encoder.decode(ptx, vec);
        print_vec(vec);
    }

    void save_data_to_csv(const std::vector<std::tuple<size_t, double>> &data,
                          const std::string &filename,
                          bool include_header) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error(filename);
        }
        file << std::fixed << std::setprecision(15);
        if (include_header) {
            file << "Index,Value\n";
        }
        for (const auto &[index, value]: data) {
            file << index << "," << value << "\n";
        }
        file.close();
    }

    void generate_normalized_vector(float *ptr, size_t dimension, std::mt19937 &gen) {
//        std::uniform_real_distribution<float> dist(0.0, 1.0);
        std::normal_distribution<float> dist(0.0, 1.0);

        float norm_sq = 0.0;

        for (int i = 0; i < dimension; ++i) {
            float val = dist(gen);
            norm_sq += val * val;
            *(ptr + i) = val;
        }

        const float norm = std::sqrt(norm_sq);
        if (norm > 0.0) {
            std::for_each(ptr, ptr + dimension, [norm](float &v) { v /= norm; });
        } else {
            ptr[0] = 1.0;
        }
    }

    void generate_random_vectors(size_t size, size_t dimension, std::vector<float> &vectors) {
        vectors.resize(size * dimension);
        auto vectors_ptr = vectors.data();
        std::random_device rd;
        std::vector<std::mt19937> gens;

        int thread_num = Params::data_gen_thread_num;
        for (int i = 0; i < thread_num; ++i) {
            gens.emplace_back(rd());
        }

        auto procFunc = [&](size_t idx, size_t thread_idx) {
            generate_normalized_vector(vectors_ptr + (idx * dimension), dimension, gens[thread_idx]);
        };
        auto consFunc = [](size_t idx) -> void {
//            std::cout << "random " << idx << " generate finished" << std::endl;
        };
        TaskManager taskManager(size, thread_num, procFunc, consFunc);

        taskManager.start();
        taskManager.join();
    }

} // util