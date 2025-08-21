// by sakara
#ifndef BIOAUTH_SECURE_AUTHENTICATION_CONFIG_H
#define BIOAUTH_SECURE_AUTHENTICATION_CONFIG_H

#include <string>
#include <cstddef>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <iostream>

namespace bioauth::experiments::secure_authentication {

const std::string kJobName = "SecureAuthentication";
constexpr std::size_t dim = 1024;      // vector length
constexpr std::size_t dbsize = 512;    // database size

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


inline void setupNetwork(const NetworkConfig& config) {
#ifdef __linux__
    if (config.delay_ms == 0) {
        int result = system("sudo tc qdisc del dev lo root 2>/dev/null");
        if (result != 0) {
            std::cerr << "Warning: Failed to reset network configuration" << std::endl;
        }
        return;
    }
    
    std::string cmd = 
        "sudo tc qdisc del dev lo root 2>/dev/null; "
        "sudo tc qdisc add dev lo root handle 1: netem delay " + 
        std::to_string(config.delay_ms) + "ms; "
        "sudo tc qdisc add dev lo parent 1:1 handle 10: tbf rate " + 
        std::to_string(config.bandwidth_mbps) + "mbit burst 32kbit latency 50ms";
    
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cerr << "Warning: Failed to setup network configuration for " << config.name << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
#elif defined(__APPLE__)
    std::this_thread::sleep_for(std::chrono::milliseconds(config.delay_ms / 10));
#else
    std::cout << "Network configuration not supported on this platform" << std::endl;
#endif
}

inline void cleanupNetwork() {
#ifdef __linux__
    int result = system("sudo tc qdisc del dev lo root 2>/dev/null");
    if (result != 0) {
        std::cerr << "Warning: Failed to cleanup network configuration" << std::endl;
    }
#elif defined(__APPLE__)
#else
#endif
}

inline std::vector<NetworkConfig> getNetworkConfigurations() {
    return {
        NetworkConfig("LAN", 1, 1000, "LAN: RTT 2ms, 1Gbps"),
        NetworkConfig("MAN", 10, 100, "MAN: RTT 20ms, 100Mbps"),
        NetworkConfig("WAN", 50, 50, "WAN: RTT 100ms, 50Mbps"),
    };
}


inline void printNetworkTestHeader(const NetworkConfig& net) {
    std::cout << "\n" << std::string(10, '-') << net.description << std::string(10, '-') <<std::endl;
    
   // std::cout << "Network: " << net.name << " (" << net.description << ")" << std::endl;
}

inline void printTestCompletion() {
    std::cout << "\n" << std::string(120, '-')  << "All network configurations tested" << std::string(120, '-')<<std::endl;
}

inline void handleTestCleanup() {
    cleanupNetwork();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

namespace FSS_Config {
    constexpr size_t DEFAULT_ROUNDS = 256;
    constexpr uint32_t FSS_NUM_BITS = 10;
    constexpr uint32_t FSS_NUM_PARTIES = 2;
    constexpr uint64_t TEST_VALUE_A = 3;
    constexpr uint64_t TEST_VALUE_B0 = 1;
    constexpr uint64_t TEST_VALUE_B1 = -1;
}

#endif // BIOAUTH_VECTOR_SELECTION_CONFIG_H
