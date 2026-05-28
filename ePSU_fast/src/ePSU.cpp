#include "ePSU.h"
#include "StdLog.h"

void ePSU(u32 idx, u32 numElements, std::vector<block> &set, std::vector<block> &ePSU_Out, Socket &chl, u32 numThreads){

    BitVector ssPMT_out(numElements);

    if(idx == 0){
        ssPMT(0, numElements, set, ssPMT_out, chl);
        ssOTd(0, numElements, set, ssPMT_out, ePSU_Out, chl, 1);
    }else if(idx == 1){
        StdLog log(idx, std::log2(numElements), "ePSU-Fast");
        log.printHeader();

        ssPMT(1, numElements, set, ssPMT_out, chl);

        Timer timer;
        auto __t0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();
        ssOTd(1, numElements, set, ssPMT_out, ePSU_Out, chl, 1);
        auto __t1 = timer.setTimePoint("ssOTd");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t1 - __t0).count();
        log.record("ssOTd", timeMs, commMB);

        log.printSummary();
    }
}
