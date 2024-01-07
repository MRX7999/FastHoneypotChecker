#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <boost/asio.hpp>
#include <fstream>
#include <unordered_map>
#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
#include <chrono>

using boost::asio::ip::tcp;
using json = nlohmann::json;

std::unordered_map<std::string, std::vector<unsigned short>> open_ports;
std::mutex mutex;

bool is_valid_ip(const std::string& ip) {
    boost::system::error_code ec;
    boost::asio::ip::make_address(ip, ec);
    return !ec;
}

void check_open_port(const std::string& ip, unsigned short port) {
    if (!is_valid_ip(ip)) {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        return;
    }

    boost::asio::io_service io_service;
    tcp::socket socket(io_service);
    tcp::endpoint endpoint(boost::asio::ip::make_address(ip), port);

    boost::system::error_code ec;
    socket.connect(endpoint, ec);

    if (!ec) {
        std::lock_guard<std::mutex> lock(mutex);
        open_ports[ip].push_back(port);
        std::cout << "Port " << port << " is open on " << ip << std::endl;
    }

    socket.close();
}

void scan_and_save(const std::vector<std::string>& ips, const std::string& json_file) {
    for (const auto& ip : ips) {
        for (unsigned short port = 1; port <= 65535; ++port) {
            check_open_port(ip, port);
        }
    }
    
    json j;
    {
        std::lock_guard<std::mutex> guard(mutex);
        for (const auto& ip : ips) {
            if (open_ports.count(ip) > 0) {
                for(const auto& port : open_ports[ip]) {
                    j[ip]["ports"][std::to_string(port)] = "open";
                }
            }
        }
    }

    std::ofstream o(json_file); 
    if (o.is_open()) {
        o << j.dump(4) << std::endl;
        o.close();
        std::cout << "JSON file for " << ips.size() << " IP addresses saved successfully!" << std::endl;
    } else {
        std::cerr << "Error opening output file." << std::endl;
    }
}

int main(int argc, char **argv)
{
    cxxopts::Options options("PortScanner", "A multithreaded port scanner");

    std::string ip_file = "ips.txt";
    std::string json_file = "results.json";
    unsigned max_threads = std::thread::hardware_concurrency();

    options.add_options()
    ("f,file", "File to read IPs from", cxxopts::value<std::string>(ip_file))
    ("o,output", "Output JSON File", cxxopts::value<std::string>(json_file))
    ("t,threads", "Number of threads", cxxopts::value<unsigned>(max_threads));

    auto result = options.parse(argc, argv);

    std::vector<std::string> ip_addresses;
    std::ifstream file(ip_file);
    std::string line;
    while (std::getline(file, line))
        ip_addresses.push_back(line);

    std::vector<std::thread> workers(max_threads);
    std::vector<std::vector<std::string>> work_loads(max_threads);

    for(size_t i = 0; i < ip_addresses.size(); ++i) {
        work_loads[i%max_threads].push_back(ip_addresses[i]);
    }

    for(size_t t = 0; t < max_threads; ++t) {
        workers[t] = std::thread(scan_and_save, std::move(work_loads[t]), json_file);
    }

    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }

    return 0;
}