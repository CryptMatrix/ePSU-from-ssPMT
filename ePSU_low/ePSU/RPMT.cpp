#include "RPMT.h"
#include "StdLog.h"

void RPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, std::vector<u32> &pi, Socket &chl, u32 numThreads){

    std::vector<block> permutedX0;
    psPMT(idx, numElements, set, out, permutedX0, pi, chl, numThreads);
    if(idx == 0){
        coproto::sync_wait(chl.send(out));
    }else if(idx == 1){
        BitVector out1(numElements);
        coproto::sync_wait(chl.recv(out1));
        for(u32 i = 0; i < numElements; i++){
            out[i]=out[i]^out1[i];
        }
    }
}


void PSU(u32 idx, u32 numElements, std::vector<block> set, std::vector<block> &PSU_Out, Socket &chl, u32 numThreads){
    BitVector peqt_out(numElements);
    std::vector<u32> pi;

    if(idx == 0){
        RPMT(0, numElements, set, peqt_out, pi, chl, numThreads);
        OT(0, numElements, set, peqt_out, PSU_Out, chl, numThreads);
    }else if(idx == 1){
        StdLog log(idx, std::log2(numElements), "PSU-Low");
        log.printHeader();

        RPMT(1, numElements, set, peqt_out, pi, chl, numThreads);

        Timer timer;
        auto __t0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();
        OT(1, numElements, set, peqt_out, PSU_Out, chl, numThreads);
        auto __t1 = timer.setTimePoint("OT");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t1 - __t0).count();
        log.record("OT", timeMs, commMB);

        log.printSummary();
    }
}
