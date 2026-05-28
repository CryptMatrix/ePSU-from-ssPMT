#include "ePSU.h"
#include "StdLog.h"

void ePSU(u32 idx, u32 numElements, std::vector<block> &set, std::vector<block> &ePSU_Out, Socket &chl, u32 numThreads){
    BitVector psPMT_Out;
    std::vector<block> permutedX0;
    std::vector<u32> pi;

    if(idx == 0){
        psPMT(idx, numElements, set, psPMT_Out, permutedX0, pi, chl, numThreads);
        ssOTd(idx, numElements, permutedX0, psPMT_Out, ePSU_Out, chl, numThreads);
    }else{
        StdLog log(idx, std::log2(numElements), "ePSU-Low");
        log.printHeader();

        psPMT(idx, numElements, set, psPMT_Out, permutedX0, pi, chl, numThreads);

        Timer timer;
        auto __t0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();
        ssOTd(idx, numElements, permutedX0, psPMT_Out, ePSU_Out, chl, numThreads);
        auto __t1 = timer.setTimePoint("ssOTd");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t1 - __t0).count();
        log.record("ssOTd", timeMs, commMB);

        log.printSummary();
    }
}
