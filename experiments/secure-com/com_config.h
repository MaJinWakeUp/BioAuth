//by sakara
#ifndef COM_CONFIG_H
#define COM_CONFIG_H

#include <string>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <iostream>

/**
 * Network configuration structure for FSS experiments
 * Defines network parameters for different network types (LAN/MAN/WAN)
 */
struct NetworkConfig {
    std::string name;
    int delay_ms;       
    int bandwidth_mbps; 
    std::string description;
    
    NetworkConfig(const std::string& n, int delay, int bw, const std::string& desc)
        : name(n), delay_ms(delay), bandwidth_mbps(bw), description(desc) {}
};

/**
 * Setup network configuration using Linux traffic control (tc)
 * Requires sudo privileges to modify network settings
 * 
 * @param config Network configuration to apply
 */
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

/**
 * Clean up network configuration by removing all tc rules
 */
inline void cleanupNetwork() {
    system("sudo tc qdisc del dev lo root 2>/dev/null");
}

/**
 * Get predefined network configurations for testing
 * 
 * @return Vector of network configurations (LAN, MAN, WAN)
 */
inline std::vector<NetworkConfig> getNetworkConfigurations() {
    return {
        NetworkConfig("BASELINE", 0, 0, "Baseline Performance Test"),
        NetworkConfig("LAN", 1, 1000, "LAN: RTT 0.1ms, 1Gbps"),
        NetworkConfig("MAN", 6, 100, "MAN: RTT 6ms, 100Mbps"),
        NetworkConfig("WAN", 80, 40, "WAN: RTT 80ms, 40Mbps"),
    };
}

/**
 * Print network configuration test header
 * 
 * @param net Network configuration being tested
 */
inline void printNetworkTestHeader(const NetworkConfig& net, bool network_configured = false) {
    std::cout << "\n--- " << net.description << " ---" << std::endl;
}

/**
 * Print final test completion message
 */
inline void printTestCompletion() {
    std::cout << "\n" << std::string(20, '-')  << "All network configurations tested" << std::string(20, '-')<<std::endl;
}

/**
 * Handle network cleanup and inter-test delay
 */
inline void handleTestCleanup() {
    cleanupNetwork();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

/**
 * Common FSS test configuration constants
 */
namespace FSS_Config {
    constexpr size_t DEFAULT_ROUNDS = 512;
    constexpr uint32_t FSS_NUM_BITS = 10;
    constexpr uint32_t FSS_NUM_PARTIES = 2;
    constexpr uint64_t TEST_VALUE_A = 3;
    constexpr uint64_t TEST_VALUE_B0 = 1;
    constexpr uint64_t TEST_VALUE_B1 = -1;
}

#endif // COM_CONFIG_H