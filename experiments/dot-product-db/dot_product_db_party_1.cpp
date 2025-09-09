// by sakara
#include "dot_product_db_config.h"
#include "share/Spdz2kShare.h"
#include "protocols/Circuit.h"
#include "utils/print_vector.h"
#include "utils/rand.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace std;
using namespace bioauth;
using namespace bioauth::experiments::dot_product;

int main() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto networks = getNetworkConfigurations();
    
    std::cout << "\n=== Dot Product Database - Party 1 ===" << std::endl;
    std::cout << "Vector length: " << dim << ", Database size: " << dbsize << std::endl;
    std::cout << std::endl;
    
    for (size_t test = 0; test < networks.size(); test++) {
        try {
            
            using ShrType = Spdz2kShare64;
            using ClearType = ShrType::ClearType;
            
            PartyWithFakeOffline<ShrType> party(1, 2, 5050, kJobName);
            Circuit<ShrType> circuit(party);
            
            auto a = circuit.input(0, 1, dim);
            auto b = circuit.input(1, dim, dbsize);
            auto c = circuit.multiply(a, b);
            auto d = circuit.output(c);
            circuit.addEndpoint(d);
            
            std::vector<ClearType> vec_b(dim * dbsize);
            std::generate(vec_b.begin(), vec_b.end(), [] { 
                return getRand<ClearType>(); 
            });
            b->setInput(vec_b);
            
            circuit.readOfflineFromFile();
            circuit.runOnlineWithBenckmark();
            
            double comm1 = party.bytes_sent_actual() / 1024.0 ;
            double time1 = circuit.timer().elapsed();
            
            std::cout << "\n--- Results for " << networks[test].name << " (Party 1) ---" << std::endl;
            std::cout << "Time: " << std::fixed << std::setprecision(2) << time1 << " ms" << std::endl;
            std::cout << "Communication: " << std::fixed << std::setprecision(2) << comm1 << " KB (" << comm1/1024.0 << " MB)" << std::endl;
                     
        } catch (const std::exception& e) {
            std::cout << "Error [" << networks[test].name << "]: " << e.what() << std::endl;
        }
        
        if (test < networks.size() - 1) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    std::cout << "All tests completed." << std::endl;
    return 0;
}