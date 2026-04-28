#include <iostream>
#include <vector>
#include <chrono>

#include "fhe_adapter.h"
#include "params.h"
#include "client.h"
#include "server.h"

std::array<double, 4> benchmark(std::array<size_t, 3> bench_params) {

    size_t db_size = bench_params[0];
    size_t query_size = bench_params[1];
    size_t cluster_num = bench_params[2];

    size_t poly_modulus_degree = Params::poly_modulus_degree;

    fhe::EncryptionParameters params(fhe::scheme_type::ckks);
    params.set_poly_modulus_degree(poly_modulus_degree);
    std::vector<int> modulus_bit_sizes(Params::modulus_bit_sizes.begin(), Params::modulus_bit_sizes.end());
    params.set_coeff_modulus(fhe::CoeffModulus::Create(poly_modulus_degree, modulus_bit_sizes));

    fhe::Context context(params);

    Client client(context, cluster_num);
    Server server(context, cluster_num);

    // ********** Initialization **********
    auto start_time = std::chrono::high_resolution_clock::now();

    std::cout << "Generating Server data......" << std::endl;
//    server.read_data("../data/database.npy");
    server.generate_random_data(db_size);
    std::cout << "Clustering Server data......" << std::endl;
    server.convert_data_to_clusters();

//    client.read_data("../data/query.npy");
    std::cout << "Generating Client data......" << std::endl;
    client.generate_random_data(query_size);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Initialization time: " << elapsed_time.count() * 1.0 / 1000 << "s." << std::endl;
    // ********** Initialization **********

    // ********** Query generation **********
    start_time = std::chrono::high_resolution_clock::now();

    const std::vector<float> &centroids = server.get_centroids();
    std::cout << "Generating Query......" << std::endl;
    auto query = client.generate_query(centroids);

    end_time = std::chrono::high_resolution_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Query generation time: " << elapsed_time.count() * 1.0 / 1000 << "s." << std::endl;
    // ********** Query generation **********

    std::cout << "Calculating query size...... " << std::endl;
    auto query_comm = query.calculate_total_size() + centroids.size() * sizeof(float);

    // ********** Response generation **********
    start_time = std::chrono::high_resolution_clock::now();

    std::cout << "Generating Response......" << std::endl;
    auto response = server.generate_response(query);

    end_time = std::chrono::high_resolution_clock::now();
    auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Response generation time: " << response_time.count() * 1.0 / 1000 << "s." << std::endl;
    // ********** Response generation **********

    std::cout << "Calculating response size...... " << std::endl;
    auto response_comm = response.calculate_total_size();

    // ********** Response decode **********
    start_time = std::chrono::high_resolution_clock::now();

    std::cout << "Response decoding......" << std::endl;
    end_time = std::chrono::high_resolution_clock::now();

    auto max_results = client.decode_response(query, response, context);

    elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Response decode time: " << elapsed_time.count() * 1.0 / 1000 << "s." << std::endl;
    // ********** Response decode **********

    double query_f = ((double) query_comm) / query_size;
    double response_f = ((double) response_comm) / query_size;

    double query_comm_size = query_f / 1024 / 1024;
    double response_comm_size = response_f / 1024 / 1024;
    double total_comm_size = (query_f + response_f) / 1024 / 1024;
    double qps = (double) query_size / ((double) response_time.count() / 10000 / 1000) / 1000;

    return {query_comm_size, response_comm_size, total_comm_size, qps};
}

void print_result(std::array<size_t, 3> param, std::array<double, 4> result) {
    std::cout << "******************************************" << std::endl;
    std::cout << "Results for: db_size = " << param[0] << ", query_size = " << param[1] << ", cluster_num = "
              << param[2] << "." << std::endl;
    std::cout << "Query size is " << result[0] << " MB." << std::endl;
    std::cout << "Response size is " << result[1] << " MB." << std::endl;
    std::cout << "Total size is " << result[2] << " MB." << std::endl;
    std::cout << "QPS: " << result[3] << "k." << std::endl;
    std::cout << "******************************************" << std::endl;
}

int main() {
    std::vector<std::array<size_t, 3>> params = {
            // db_size, query_size, cluster_num

            /************ K = 256 ************/
            {1000000,   500000,  256},
//            {1000000,   1000000, 256},
//            {1000000,   2000000, 256},
//            {1000000,   5000000, 256},
//
//            {16000000,  500000,  256},
//            {16000000,  1000000, 256},
//            {16000000,  2000000, 256},
//            {16000000,  5000000, 256},

//            {100000000, 500000,  256},
//            {100000000, 1000000, 256},
//            {100000000, 2000000, 256},
//            {100000000, 5000000, 256},
            /************ K = 256 ************/

            /************ K = 512 ************/
//            {1000000,   500000,  512},
//            {1000000,   1000000, 512},
//            {1000000,   2000000, 512},
//            {1000000,   5000000, 512},

//            {16000000,  500000,  512},
//            {16000000,  1000000, 512},
//            {16000000,  2000000, 512},
//            {16000000,  5000000, 512},
//
//            {100000000, 500000,  512},
//            {100000000, 1000000, 512},
//            {100000000, 2000000, 512},
//            {100000000, 5000000, 512},
            /************ K = 512 ************/

            /************ K = 1024 ************/
//            {1000000,   500000,  1024},
//            {1000000,   1000000, 1024},
//            {1000000,   2000000, 1024},
//            {1000000,   5000000, 1024},

//            {16000000,  500000,  1024},
//            {16000000,  1000000, 1024},
//            {16000000,  2000000, 1024},
//            {16000000,  5000000, 1024},
//
//            {100000000, 500000,  1024},
//            {100000000, 1000000, 1024},
//            {100000000, 2000000, 1024},
//            {100000000, 5000000, 1024},
            /************ K = 1024 ************/
    };

    std::vector<std::array<double, 4>> results;
    results.reserve(params.size());
    for (auto &param: params) {
        auto result = benchmark(param);
        results.push_back(result);
        print_result(param, result);
    }

    std::cout << "\n\n" << std::endl;
    for (int i = 0; i < params.size(); ++i) {
        auto &param = params[i];
        auto &result = results[i];
        print_result(param, result);
    }
}


