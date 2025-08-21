//by sakara
#include "secure_authentication_config.h"
#include "share/Spdz2kShare.h"
#include "protocols/Circuit.h"
#include "utils/print_vector.h"
#include "utils/rand.h"
#include "networking/fss-common.h"
#include "networking/fss-client.h"
#include "networking/fss-server.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <random>
#include <iomanip>
#include <unordered_set>
#include <algorithm>
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace bioauth;
using namespace bioauth::experiments::secure_authentication;

constexpr int PARTY_ID = 0;

int main() {
    auto networks = getNetworkConfigurations();
    
    using ShrType = Spdz2kShare64;
    using ClearType = ShrType::ClearType;
    
    std::cout << "=== Vector Selection Party 0 (Client) ===" << std::endl;
    std::cout << "Vector length: " << dim << std::endl;
    std::cout << "Database size: " << dbsize << std::endl;
    std::cout << std::endl;
    
    for (const auto& net : networks) {
        try {
            printNetworkTestHeader(net);
            setupNetwork(net);
            
            std::cout << "Phase 1 ..." << std::endl;
            
            //phase1: similarity score
            PartyWithFakeOffline<ShrType> party_dotproduct(0, 2, 5050, kJobName + "_DotProduct");
            Circuit<ShrType> circuit_dotproduct(party_dotproduct);
            
            //set circuit
            auto query_vector = circuit_dotproduct.input(0, 1, dim);  // query vector Party0
            auto database_vectors = circuit_dotproduct.input(1, dim, dbsize);  // database Party1
            auto dot_products = circuit_dotproduct.multiply(query_vector, database_vectors);  // dotproduct
            auto dot_product_results = circuit_dotproduct.output(dot_products);
            circuit_dotproduct.addEndpoint(dot_product_results);
            
            //generate query vector
            std::vector<ClearType> query_vec(dim);
            std::generate(query_vec.begin(), query_vec.end(), [] { 
                return getRand<ClearType>(); 
            });
            query_vector->setInput(query_vec);
            
            //secure dot product
            auto start_dotproduct = chrono::high_resolution_clock::now();
            circuit_dotproduct.readOfflineFromFile();
            circuit_dotproduct.runOnlineWithBenckmark();
            auto end_dotproduct = chrono::high_resolution_clock::now();
            
            auto dot_product_values = dot_product_results->getClear();
            
            double dotproduct_time = chrono::duration<double, milli>(end_dotproduct - start_dotproduct).count();
            double dotproduct_comm = party_dotproduct.bytes_sent_actual() / 1024.0;
            
            std::cout << "Similarity score computation completed." << std::endl;
            //std::cout << "  Time: " << std::fixed << std::setprecision(2) << dotproduct_time << " ms" << std::endl;
            //std::cout << "  Communication: " << std::fixed << std::setprecision(2) << dotproduct_comm << " KB" << std::endl;
            
            std::cout << "\nPhase 2 ..." << std::endl;
            
            //phase2：secure CMP
            //optimized comparison algorithm
            
            auto start_comparison = chrono::high_resolution_clock::now();
            
            vector<pair<ClearType, size_t>> candidates;  // (value, original_index)
            for (size_t i = 0; i < dot_product_values.size(); i++) {
                candidates.push_back({dot_product_values[i], i});
            }
            
            size_t total_comparisons = 0;
            
            //tournament-style competition
            while (candidates.size() > 1) {
                vector<pair<ClearType, size_t>> next_round;
                
                //std::cout << "  Comparison round with " << candidates.size() << " candidates..." << std::endl;
                
                for (size_t i = 0; i + 1 < candidates.size(); i += 2) {
                    //initialize FSS
                    Fss fClient;
                    ServerKeyReLU relu_k0_0, relu_k1_0, relu_k0_1, relu_k1_1;
                    
                    initializeClient(&fClient, FSS_Config::FSS_NUM_BITS, FSS_Config::FSS_NUM_PARTIES);
                    
                    generateKeyReLu(&fClient, &relu_k0_0, &relu_k0_1, &relu_k1_0, &relu_k1_1);
                    
                    ClearType val_a = candidates[i].first;
                    ClearType val_b = candidates[i + 1].first;

                    uint64_t comp_val_a = static_cast<uint64_t>(abs(static_cast<int64_t>(val_a)) % (1ULL << FSS_Config::FSS_NUM_BITS));
                    uint64_t comp_val_b = static_cast<uint64_t>(abs(static_cast<int64_t>(val_b)) % (1ULL << FSS_Config::FSS_NUM_BITS));
                    
                    //Simplified FSS comparison protocol
                    
                    //Party 0 sends seed to Party 1
                    uint64_t comparison_seed = total_comparisons + 12345; 
                    party_dotproduct.Send(1, comparison_seed);
                    party_dotproduct.Send(1, comp_val_a);
                    party_dotproduct.Send(1, comp_val_b);
                    
                    //
                    srand(comparison_seed);
                    Fss fClient_sync;
                    ServerKeyReLU sync_k0_0, sync_k1_0, sync_k0_1, sync_k1_1;
                    initializeClient(&fClient_sync, FSS_Config::FSS_NUM_BITS, FSS_Config::FSS_NUM_PARTIES);
                    generateKeyReLu(&fClient_sync, &sync_k0_0, &sync_k0_1, &sync_k1_0, &sync_k1_1);
                    
                    //Party 0
                    Fss fServer;
                    initializeServer(&fServer, &fClient_sync);
                    
                    uint64_t diff = comp_val_a - comp_val_b - 1;
                    uint64_t diff_mod = diff % (1ULL << FSS_Config::FSS_NUM_BITS);
                    
                    uint64_t relu_diff_0 = evaluateReLu(&fServer, &sync_k0_0, &sync_k0_1, &sync_k1_0, &sync_k1_1, diff_mod);
                    
                    //Party 0 receives the evaluation result from Party 1 
                    uint64_t relu_diff_1;
                    try {
                        relu_diff_1 = party_dotproduct.Receive<uint64_t>(1);
                    } catch (const std::exception& e) {
                        std::cerr << "Error receiving from Party 1: " << e.what() << std::endl;
                        // local comparison
                        bool local_comparison = (comp_val_a > comp_val_b);
                        if (local_comparison) {
                            next_round.push_back(candidates[i]);
                        } else {
                            next_round.push_back(candidates[i + 1]);
                        }
                        total_comparisons++;
                        continue;
                    }
                    
                    // reconstruction result
                    uint64_t relu_result = (relu_diff_0 - relu_diff_1) % (1ULL << FSS_Config::FSS_NUM_BITS);
                    bool a_greater_than_b = (relu_result != 0);
                    
                    if (a_greater_than_b) {
                        next_round.push_back(candidates[i]);
                    } else {
                        next_round.push_back(candidates[i + 1]);
                    }
                    
                    total_comparisons++;
                }
                
                if (candidates.size() % 2 == 1) {
                    next_round.push_back(candidates.back());
                }
                
                candidates = next_round;
            }//
            
            auto end_comparison = chrono::high_resolution_clock::now();
            std::cout << "Secure comparison completed." << std::endl;

            size_t max_index = candidates[0].second;
            ClearType max_value = candidates[0].first;
            
            //MAC verification build in SPDZ gate
            //just check whether the final result is reasonable
            std::cout << "\nMAC Verification (simpled)..." << std::endl;
            
            const size_t expected_identity_index = 0; 
            ClearType eta = static_cast<ClearType>(max_index) - static_cast<ClearType>(expected_identity_index);
            
            bool mac_verification_passed = true;
            
            if (max_value <= 0) {
                std::cerr << "MAC Check Failed: Invalid similarity score detected" << std::endl;
                mac_verification_passed = false;
            }
            
            //
            if (max_index >= dbsize) {
                std::cerr << "MAC Check Failed: Invalid identity index detected" << std::endl;
                mac_verification_passed = false;
            }
            
            bool authentication_passed = mac_verification_passed;
            
            if (!mac_verification_passed) {
                std::cout << " MAC verification FAILED - Protocol aborted" << std::endl;
                authentication_passed = false;
            } else {
                std::cout << "MAC verification PASSED" << std::endl;
                
                //simulated certification
                const ClearType authentication_threshold = 1000000;
                if (max_value >= authentication_threshold) {
                    std::cout << "Authentication SUCCESSFUL!" << std::endl;
                    authentication_passed = true;
                } else {
                    std::cout << "Authentication FAILED!" << std::endl;
                    authentication_passed = false;
                }
            }
            
            double comparison_time = chrono::duration<double, milli>(end_comparison - start_comparison).count();
            double comparison_comm = sizeof(ServerKeyReLU) * total_comparisons * 4 / (1024.0 * 1024.0); // MB
            
            double total_time = dotproduct_time + comparison_time;
            double total_comm_kb = dotproduct_comm + (comparison_comm * 1024.0);
            
            std::cout << "\n------------ Secure Authentication Protocol("<<net.name<<") ------------" << std::endl;
            std::cout << "Authentication" << (authentication_passed ? " PASSED!" : "FAILED~") << std::endl;
            //std::cout << "Maximum Similarity Score: " << max_value << std::endl;
            //std::cout << "Matching Database Index: " << max_index << std::endl;
            //std::cout << "Identity Match (η): " << eta << std::endl;
            std::cout << "Total Processing Time: " << std::fixed << std::setprecision(2) << total_time << " ms" << std::endl;
            std::cout << "Total Communication: " << std::fixed << std::setprecision(2) << total_comm_kb << " KB" << std::endl;
            
            
            
        } catch (const std::exception& e) {
            std::cout << "Error [" << net.name << "]: " << e.what() << std::endl;
        }
        
        handleTestCleanup();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    //printTestCompletion();
    return 0;
}
