// by sakara
#ifndef BIOAUTH_DOT_PRODUCT_CONFIG_H
#define BIOAUTH_DOT_PRODUCT_CONFIG_H

#include <string>
#include <cstddef>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <iostream>

namespace bioauth::experiments::dot_product {

const std::string kJobName = "DotProduct";
constexpr std::size_t dim = 1024;
constexpr std::size_t dbsize = 512;

}

//network
 
struct NetworkConfig {
    std::string name;
    int delay_ms;       
    int bandwidth_mbps; 
    std::string description;
    
    NetworkConfig(const std::string& n, int delay, int bw, const std::string& desc)
        : name(n), delay_ms(delay), bandwidth_mbps(bw), description(desc) {}
};


inline bool setupNetwork(const NetworkConfig& config) {
    system("tc qdisc del dev lo root 2>/dev/null");
    
    if (config.delay_ms == 0) {
        return true; // Baseline
    }
    
    std::string cmd = 
        "tc qdisc add dev lo root handle 1: tbf rate " + 
        std::to_string(config.bandwidth_mbps) + "mbit burst 100000 limit 10000 && " +
        "tc qdisc add dev lo parent 1:1 handle 10: netem delay " + 
        std::to_string(config.delay_ms) + "ms";
    
    int result = system(cmd.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return (result == 0);
}

inline void cleanupNetwork() {
    system("tc qdisc del dev lo root 2>/dev/null");
}

inline std::vector<NetworkConfig> getNetworkConfigurations() {
    return {
        NetworkConfig("BASELINE", 0, 0, "Baseline Performance Test"),
        NetworkConfig("LAN", 1, 1000, "LAN: RTT 0.1ms, 1Gbps"),
        NetworkConfig("MAN", 6, 100, "MAN: RTT 6ms, 100Mbps"),
        NetworkConfig("WAN", 80, 40, "WAN: RTT 80ms, 40Mbps"),
    };
}

inline void printNetworkTestHeader(const NetworkConfig& net, bool network_configured = false) {
    std::cout << "\n--- " << net.description << " ---" << std::endl;
}

inline void printTestCompletion() {
    std::cout << "\n" << std::string(120, '-')  << "All network configurations tested" << std::string(120, '-')<<std::endl;
}

inline void handleTestCleanup() {
    cleanupNetwork();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

#endif 