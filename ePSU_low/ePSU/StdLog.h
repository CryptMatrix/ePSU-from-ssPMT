#pragma once

#include <cryptoTools/Common/Timer.h>
#include <coproto/Socket/AsioSocket.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>

struct ModuleStat {
    std::string name;
    double timeMs;
    double commMB;
};

class StdLog {
public:
    StdLog(u32 idx, u32 nn, const std::string& protocol)
        : mIdx(idx), mNn(nn), mNumElements(1u << nn), mProtocol(protocol)
    {
        mStartWall = std::chrono::high_resolution_clock::now();
        gInstance = this;
    }

    ~StdLog() { if (gInstance == this) gInstance = nullptr; }

    static StdLog* instance() { return gInstance; }

    void record(const std::string& name, double timeMs, double commMB) {
        mStats.push_back({name, timeMs, commMB});
        printStatLine(name, timeMs, commMB);
    }

    void printHeader() {
        std::cout << std::endl;
        std::cout << "========================================================" << std::endl;
        std::cout << "  Protocol: " << mProtocol
                  << " | Role: Party " << mIdx
                  << " | Set Size: 2^" << mNn << " = " << mNumElements << std::endl;
        std::cout << "--------------------------------------------------------" << std::endl;
    }

    void printSummary() {
        double totalTime = 0;
        double totalComm = 0;
        auto endWall = std::chrono::high_resolution_clock::now();
        double wallMs = std::chrono::duration<double, std::milli>(endWall - mStartWall).count();

        for (auto& s : mStats) {
            totalTime += s.timeMs;
            totalComm += s.commMB;
        }

        std::cout << "--------------------------------------------------------" << std::endl;
        printStatLine("TOTAL", totalTime, totalComm);
        printStatLine("WALL CLOCK", wallMs);
        std::cout << "========================================================" << std::endl;
        std::cout << std::endl;
    }

    u32 getIdx()      const { return mIdx; }
    u32 getNn()       const { return mNn; }
    u32 numElements() const { return mNumElements; }

private:
    u32 mIdx;
    u32 mNn;
    u32 mNumElements;
    std::string mProtocol;
    std::vector<ModuleStat> mStats;
    std::chrono::high_resolution_clock::time_point mStartWall;
    static inline thread_local StdLog* gInstance = nullptr;

    static void printStatLine(const std::string& name, double timeMs, double commMB) {
        printStatPrefix(name, timeMs);
        std::cout << " | "
                  << std::setw(kValueWidth) << commMB << " MB" << std::endl;
    }

    static void printStatLine(const std::string& name, double timeMs) {
        printStatPrefix(name, timeMs);
        std::cout << std::endl;
    }

    static void printStatPrefix(const std::string& name, double timeMs) {
        std::cout << std::fixed << std::setprecision(3)
                  << "  " << std::left << std::setw(kLabelWidth)
                  << ("[" + name + "]")
                  << std::right << std::setw(kValueWidth)
                  << timeMs << " ms";
    }

    static constexpr int kLabelWidth = 14;
    static constexpr int kValueWidth = 10;
};
