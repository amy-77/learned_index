//
//  main.cpp
//  bench_search
//
//  Created by Liu Qiyu on 2024/8/17.
//
#include <cassert> // 添加 assert 头文件
#include <random>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <limits>
#include "search_algo.h"
#include "utils.h"
// #include "rmi.h"
#include "fb_200M_uint64_2.h"
#include "fb_200M_uint64_2_data.h"

bool file_exists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

//加速优化版本的zipfan采样
// 生成均匀随机数
double rand_val(int seed) {
    static bool first = true; // 使用 true 和 false
    if (first) {
        srand(seed == 0 ? time(NULL) : seed);
        first = false;
    }
    return ((double) rand() / (double) RAND_MAX);
}

// 生成 Zipfian 分布的值
int zipf(double alpha, int n) {
    static bool first = true;      // 使用 true 和 false
    static double c = 0;          // Normalization constant
    static double *sum_probs;     // Pre-calculated sum of probabilities
    double z;                     // Uniform random number (0 < z < 1)
    int zipf_value;               // Computed exponential value to be returned
    int i;                        // Loop counter
    int low, high, mid;           // Binary-search bounds

    // Compute normalization constant on first call only
    if (first) {
        for (i = 1; i <= n; i++) {
            c = c + (1.0 / pow((double) i, alpha));
        }
        c = 1.0 / c;

        sum_probs = (double *)malloc((n + 1) * sizeof(*sum_probs));
        sum_probs[0] = 0;
        for (i = 1; i <= n; i++) {
            sum_probs[i] = sum_probs[i - 1] + c / pow((double) i, alpha);
        }
        first = false;
    }

    // Pull a uniform random number (0 < z < 1)
    do {
        z = rand_val(0);
    } while ((z == 0) || (z == 1));

    // Map z to the value
    low = 1, high = n, mid = 0;
    do {
        mid = (low + high) / 2;
        if (sum_probs[mid] >= z && sum_probs[mid - 1] < z) {
            zipf_value = mid;
            break;
        } else if (sum_probs[mid] >= z) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    } while (low <= high);

    // Assert that zipf_value is between 1 and N
    assert((zipf_value >= 1) && (zipf_value <= n));

    return zipf_value;
}

// 生成 Zipfian 分布的查询样本
std::vector<uint64_t> gen_zipfian_queries(const std::vector<uint64_t>& data, size_t nq, double alpha) {
    std::vector<uint64_t> queries;
    queries.reserve(nq);

    for (size_t i = 0; i < nq; ++i) {
        int index = zipf(alpha, data.size()) - 1; // 生成 Zipfian 分布的索引
        queries.push_back(data[index]);
    }

    return queries;
}

void append_results_to_csv(const std::string& filename, size_t round, size_t search_time, size_t total_time, size_t err_total, size_t err_max, size_t nq) {
    std::ofstream file;
    bool exists = file_exists(filename);
    file.open(filename, std::ios::out | std::ios::app);
    if (file.is_open()) {
        if (!exists) {
            // 写入表头
            file << "round,RMI search time,RMI total time,RMI avg error,RMI max error,RMI size\n";
        }
        // 写入数据
        file << round << "," 
             << search_time / nq << "," 
             << total_time / nq << "," 
             << err_total / nq << "," 
             << err_max << "," 
             << fb_200M_uint64_2::RMI_SIZE << "\n";
        file.close();
    } else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }
}

void run_tests(const std::vector<uint64_t>& data, size_t nq, size_t repeat, double alpha, const std::string& filename, const std::function<std::vector<uint64_t>(const std::vector<uint64_t>&, size_t)>& gen_queries) {
    for (size_t i = 0; i < repeat; ++i) {
        // 生成 nq 个 查询样本
        auto queries = gen_queries(data, nq);
        size_t search_time = 0;
        size_t total_time = 0;
        size_t err_total = 0;
        size_t err_max = 0;
        for (auto q : queries) {
            size_t err = 0;
            auto start = std::chrono::high_resolution_clock::now();
            //输入查询q，返回预测的索引res，还有误差err
            auto res = fb_200M_uint64_2::lookup(q, &err);
            auto end = std::chrono::high_resolution_clock::now();
            search_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            err_total += err;
            err_max = err > err_max ? err : err_max;
            start = std::chrono::high_resolution_clock::now();
            // 边界检查
            size_t lower_bound_index = (res > err) ? res - err : 0;
            size_t upper_bound_index = (res + err < data.size()) ? res + err : data.size() - 1;
            //查找范围 [data.begin() + res - err, data.begin() + res + err) 中第一个不小于 q 的元素
            res = *std::lower_bound(data.begin() + lower_bound_index, data.begin() + upper_bound_index, q);
            // res = *std::lower_bound(data.begin()+res-err, data.begin()+res+err, q);
            end = std::chrono::high_resolution_clock::now();
            total_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }
        total_time += search_time;

        std::cout << " Sample " << i << ": "
                  << " RMI search time: " << search_time / nq 
                  << " RMI total time: " << total_time / nq
                  << " RMI avg error: " << err_total / nq
                  << " RMI max error: " << err_max
                  << " RMI size: " << fb_200M_uint64_2::RMI_SIZE
                  << std::endl;
        
        // 保存结果到 CSV 文件
        append_results_to_csv(filename, i, search_time, total_time, err_total, err_max, nq);
    }
}


int main(int argc, const char * argv[]) {
    const std::string fname = "fb_200M_uint64"; 
    
    const size_t nq = 10000;
    const size_t repeat = 10;
    const double alpha = 1.3; // Zipfian 分布的参数

    std::cout << "Load data from " << fname << std::endl;
    auto data = benchmark::load_data<uint64_t>(fname);
    std::sort(data.begin(), data.end());
    
    auto data_stats = benchmark::get_data_stats(data);
    std::cout << "mean: " << data_stats.mean 
              << " variance: " << data_stats.var
              << " hardness ratio: " << data_stats.var/(data_stats.mean*data_stats.mean) << std::endl;
    
    // 加载RMI模型
    fb_200M_uint64_2::load("RMI_output_fb"); 

    // CSV 文件路径
    std::string zipf_filename = "result/fb_200M_uint64_zipfan_results.csv";
    std::string rand_filename = "result/fb_200M_uint64_random_results.csv";

    // 运行 Zipfian 分布的测试
    run_tests(data, nq, repeat, alpha, zipf_filename, [&](const std::vector<uint64_t>& data, size_t nq) {
        return gen_zipfian_queries(data, nq, alpha);
    });

    // 运行随机分布的测试
    run_tests(data, nq, repeat, alpha, rand_filename, [&](const std::vector<uint64_t>& data, size_t nq) {
        return benchmark::gen_random_queries(data, nq);
    });

    fb_200M_uint64_2::cleanup();
    return 0;
}

