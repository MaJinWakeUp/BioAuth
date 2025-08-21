// by sakara
#include "secure_authentication_config.h"
#include "share/Spdz2kShare.h"
#include "protocols/Circuit.h"
#include "utils/print_vector.h"
#include "utils/rand.h"
#include "networking/fss-common.h"
#include "networking/fss-server.h"
#include "networking/fss-client.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace bioauth;
using namespace bioauth::experiments::secure_authentication;

constexpr int PARTY_ID = 1;

int main() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto networks = getNetworkConfigurations();
    
    std::cout << "=== Vector Selection Party 1 (Server) ===" << std::endl;
    std::cout << "Vector length: " << dim << std::endl;
    std::cout << "Database size: " << dbsize << std::endl;
    std::cout << std::endl;
    
    for (size_t test = 0; test < networks.size(); test++) {
        const auto& net = networks[test];
        
        try {
            //std::cout << "Testing network: " << net.description << std::endl;
            
            using ShrType = Spdz2kShare64;
            using ClearType = ShrType::ClearType;
            
            std::cout << "Phase 1 ..." << std::endl;
            
            //phase 1
            PartyWithFakeOffline<ShrType> party_dotproduct(1, 2, 5050, kJobName + "_DotProduct");
            Circuit<ShrType> circuit_dotproduct(party_dotproduct);
            
            // circuit
            auto query_vector = circuit_dotproduct.input(0, 1, dim);  // 查询向量 (Party 0 输入)
            auto database_vectors = circuit_dotproduct.input(1, dim, dbsize);  // 数据库向量 (Party 1 输入)
            auto dot_products = circuit_dotproduct.multiply(query_vector, database_vectors);  // 计算内积
            auto dot_product_results = circuit_dotproduct.output(dot_products);
            circuit_dotproduct.addEndpoint(dot_product_results);
            
            // generate database
            std::vector<ClearType> database_vecs(dim * dbsize);
            std::generate(database_vecs.begin(), database_vecs.end(), [] { 
                return getRand<ClearType>(); 
            });
            database_vectors->setInput(database_vecs);
            
            //dot product
            auto start_dotproduct = chrono::high_resolution_clock::now();
            circuit_dotproduct.readOfflineFromFile();
            circuit_dotproduct.runOnlineWithBenckmark();
            auto end_dotproduct = chrono::high_resolution_clock::now();
            
            double dotproduct_time = chrono::duration<double, milli>(end_dotproduct - start_dotproduct).count();
            double dotproduct_comm = party_dotproduct.bytes_sent_actual() / 1024.0;
            
            std::cout << "Similarity score computation completed." << std::endl;
            //std::cout << "  Time: " << std::fixed << std::setprecision(2) << dotproduct_time << " ms" << std::endl;
            //std::cout << "  Communication: " << std::fixed << std::setprecision(2) << dotproduct_comm << " KB" << std::endl;
            
            //phase2
            std::cout << "\nPhase 2 ..." << std::endl;
            //optimized comparison
            
            auto start_comparison = chrono::high_resolution_clock::now();
            
            // comparison rounds
            size_t total_candidates = dbsize;
            size_t total_comparisons = 0;
            size_t current_round_candidates = total_candidates;
            
            while (current_round_candidates > 1) {
                size_t comparisons_this_round = current_round_candidates / 2;
                total_comparisons += comparisons_this_round;
                current_round_candidates = (current_round_candidates + 1) / 2; // 向上取整
            }
            
            //std::cout << "  Expected total comparisons: " << total_comparisons << std::endl;
            
            // reset
            current_round_candidates = total_candidates;
            size_t completed_comparisons = 0;
            
            // perform comparison
            while (current_round_candidates > 1) {
                size_t comparisons_this_round = current_round_candidates / 2;
                
                //std::cout << "  Comparison round with " << current_round_candidates << " candidates (" 
                         //<< comparisons_this_round << " comparisons)..." << std::endl;
                
                for (size_t i = 0; i < comparisons_this_round; i++) {
                    
                    //receve seed and values
                    uint64_t comparison_seed = party_dotproduct.Receive<uint64_t>(0);
                    uint64_t comp_val_a = party_dotproduct.Receive<uint64_t>(0);
                    uint64_t comp_val_b = party_dotproduct.Receive<uint64_t>(0);
                    
                    //generate FSS key
                    srand(comparison_seed);
                    Fss fClient_sync;
                    ServerKeyReLU sync_k0_0, sync_k1_0, sync_k0_1, sync_k1_1;
                    initializeClient(&fClient_sync, FSS_Config::FSS_NUM_BITS, FSS_Config::FSS_NUM_PARTIES);
                    generateKeyReLu(&fClient_sync, &sync_k0_0, &sync_k0_1, &sync_k1_0, &sync_k1_1);
                    
                    Fss fServer;
                    initializeServer(&fServer, &fClient_sync);
                    
                    uint64_t diff = comp_val_a - comp_val_b - 1;
                    uint64_t diff_mod = diff % (1ULL << FSS_Config::FSS_NUM_BITS);
                    
                    uint64_t relu_diff_1 = evaluateReLu(&fServer, &sync_k0_0, &sync_k0_1, &sync_k1_0, &sync_k1_1, diff_mod);
                    
                    //Party 1 sends result to Party 0
                    party_dotproduct.Send(0, relu_diff_1);
                    
                    completed_comparisons++;
                }
                
                current_round_candidates = (current_round_candidates + 1) / 2;
            }
            
            auto end_comparison = chrono::high_resolution_clock::now();
            
            double comparison_time = chrono::duration<double, milli>(end_comparison - start_comparison).count();
            double comparison_comm = sizeof(uint64_t) * total_comparisons * 3 / (1024.0 * 1024.0); // MB 
            
            double total_time = dotproduct_time + comparison_time;
            double total_comm_kb = dotproduct_comm + (comparison_comm * 1024.0);
            
            std::cout << "Secure comparison completed." << std::endl;
           // std::cout << "  Total comparisons completed: " << completed_comparisons << std::endl;
           // std::cout << "  Time: " << std::fixed << std::setprecision(2) << comparison_time << " ms" << std::endl;
            //std::cout << "  Communication: " << std::fixed << std::setprecision(4) << comparison_comm << " MB" << std::endl;
            
            std::cout << "Waiting for Party 0 to complete authentication..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            std::cout << "\n--- Secure Authentication Protocol(" << net.name << ", Party 1) ---" << std::endl;
            std::cout << "Total processing time: " << std::fixed << std::setprecision(2) << total_time << " ms" << std::endl;
            std::cout << "Total communication: " << std::fixed << std::setprecision(2) << total_comm_kb << " KB" << std::endl;
            
            
            
        } catch (const std::exception& e) {
            std::cout << "Error [" << net.name << "]: " << e.what() << std::endl;
        }
        
        if (test < networks.size() - 1) {
            std::cout << "Waiting for next test..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    std::cout << "\nAll tests completed." << std::endl;
    return 0;
}
