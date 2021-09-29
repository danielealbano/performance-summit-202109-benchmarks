#include <ios>
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

#include <benchmark/benchmark.h>

std::string GetCpuName() {
    std::string line, modelName;

    if (!std::filesystem::exists("/proc/cpuinfo")) {
        return "Unknown";
    }

    std::ifstream cpuinfo("/proc/cpuinfo", std::ios::binary);

    while (std::getline(cpuinfo, line)) {
        if (line.starts_with("model name")) {
            modelName = line.substr(line.find(':') + 2);
            break;
        }
    }

    cpuinfo.close();

    return modelName;
}

std::int32_t GetCpuCoreCount() {
    std::string line, lastProcessorNumber;
    std::int32_t coreCount;

    if (!std::filesystem::exists("/proc/cpuinfo")) {
        return -1;
    }

    std::ifstream cpuinfo("/proc/cpuinfo", std::ios::binary);

    while (std::getline(cpuinfo, line)) {
        if (line.starts_with("processor")) {
            lastProcessorNumber = line.substr(line.find(':') + 2);
        }
    }

    cpuinfo.close();

    coreCount = std::stoi(lastProcessorNumber) + 1;

    return coreCount;
}

std::int32_t GetCpuFrequency() {
    std::string line, cpuMhz;

    if (!std::filesystem::exists("/proc/cpuinfo")) {
        return -1;
    }

    std::ifstream cpuinfo("/proc/cpuinfo", std::ios::binary);

    while (std::getline(cpuinfo, line)) {
        if (line.starts_with("cpu MHz")) {
            cpuMhz = line.substr(line.find(':') + 2);
        }
    }

    cpuinfo.close();

    return std::stoi(cpuMhz);
}

std::int32_t GetNumaNodeCount() {
    std::int32_t numaNodeCount = 0;

    if (!std::filesystem::exists("/sys/devices/system/node/")) {
        return -1;
    }

    for (const auto & file : std::filesystem::directory_iterator("/sys/devices/system/node/")) {
        if (!file.is_directory() || !file.path().filename().string().starts_with("node")) {
            continue;
        }

        std::int32_t currentNumaNode = std::stoi(file.path().filename().string().substr(4));
        if (currentNumaNode <= numaNodeCount) {
            continue;
        }

        numaNodeCount = currentNumaNode;
    }

    return numaNodeCount + 1;
};


int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::AddCustomContext("CPU Name", GetCpuName());
    ::benchmark::AddCustomContext("CPU Core Count", std::to_string(GetCpuCoreCount()));
    ::benchmark::AddCustomContext("CPU Frequency", std::to_string(GetCpuFrequency()));
    ::benchmark::AddCustomContext("NUMA Node Count", std::to_string(GetNumaNodeCount()));
    ::benchmark::RunSpecifiedBenchmarks();

    return 0;
}