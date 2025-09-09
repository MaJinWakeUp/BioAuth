//by sakara
#include "dot_product_db_config.h"
#include "share/Spdz2kShare.h"
#include "protocols/Circuit.h"
#include "utils/print_vector.h"
#include "utils/rand.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <random>
#include <iomanip>
#include <unordered_set>
#include <algorithm>

using namespace std;
using namespace bioauth;
using namespace bioauth::experiments::dot_product;

constexpr int PARTY_ID = 0;

int main() {
    auto networks = getNetworkConfigurations();
    
    using ShrType = Spdz2kShare64;
    using ClearType = ShrType::ClearType;
    
    std::cout << "\n=== Dot Product Database - Party 0 ===" << std::endl;
    std::cout << "Vector length: " << dim << ", Database size: " << dbsize << std::endl;
    std::cout << std::endl;
    
    for (const auto& net : networks) {
        try {
            bool network_configured = setupNetwork(net);
            printNetworkTestHeader(net, network_configured);
            
            PartyWithFakeOffline<ShrType> party(0, 2, 5050, kJobName);
            Circuit<ShrType> circuit(party);
            
            auto a = circuit.input(0, 1, dim);
            auto b = circuit.input(1, dim, dbsize);
            auto c = circuit.multiply(a, b);
            auto d = circuit.output(c);
            circuit.addEndpoint(d);
            
            std::vector<ClearType> vec_a(dim);
            std::generate(vec_a.begin(), vec_a.end(), [] { 
                return getRand<ClearType>(); 
            });
            a->setInput(vec_a);
            
            circuit.readOfflineFromFile();
            circuit.runOnlineWithBenckmark();
            
            double comm0 = party.bytes_sent_actual() / 1024.0 ;
            double time0 = circuit.timer().elapsed();
            
            std::cout << "\n--- Results for " << net.name << " (Party 0) ---" << std::endl;
            std::cout << "Time: " << std::fixed << std::setprecision(2) << time0 << " ms" << std::endl;
            std::cout << "Communication: " << std::fixed << std::setprecision(2) << comm0 << " KB (" << comm0/1024.0 << " MB)" << std::endl;
                     
        } catch (const std::exception& e) {
            std::cout << "Error [" << net.name << "]: " << e.what() << std::endl;
        }
        
        handleTestCleanup();
    }
    
    cleanupNetwork();
    return 0;
}