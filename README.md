# Port Scanner

A multithreaded port scanner that checks open ports for multiple IP addresses and saves the results in a JSON file.

## Dependencies
- boost/asio.hpp
- nlohmann/json.hpp
- cxxopts.hpp

## Usage

The following command-line options are available:

- `-f`, `--file` : File to read IPs from (default: "ips.txt")
- `-o`, `--output` : Output JSON File (default: "results.json")
- `-t`, `--threads` : Number of threads (default: number of available hardware threads)

## How it works

The program reads in IP addresses from a text file and divides them into different workloads according to the number of threads specified. Each thread then iterates through each IP address and checks for open ports by attempting a connection on each port number between 1 and 65535. If a connection is successful, the IP address and port number are stored in an `unordered_map` guarded by a mutex.

When all threads have completed their scans, the program serializes the `unordered_map` into a JSON object and saves it to a file using the specified filename. The resulting JSON object is structured as follows:

```json
{
    "ip-address-1": {
        "ports": {
            "port-number-1": "open",
            "port-number-2": "open"
        }
    },
    "ip-address-2": {
        "ports": {
            "port-number-3": "open"
        }
    }
}
```
