#include "RPMT.h"
#include "StdLog.h"

void PEQT(u32 idx, std::vector<block>& input, BitVector& out, Socket& chl, u32 numThreads)
{
    u64 numElements = input.size();
    u32 keyBitLength = 40 + oc::log2ceil(numElements);
    u64 keyByteLength = oc::divCeil(keyBitLength, 8);

    for (auto& b : input) {
        u8* ptr = reinterpret_cast<u8*>(&b);
        for (u64 j = keyByteLength; j < 16; ++j) {
            ptr[j] = 0;
        }
        if (keyBitLength % 8 != 0) {
            u8 mask = (1 << (keyBitLength % 8)) - 1;
            ptr[keyByteLength - 1] &= mask;
        }
    }

    if (idx == 0)
    {
        RsPsiSender sender;

        sender.init(numElements, numElements, 40, ZeroBlock, false, numThreads);

        coproto::sync_wait(sender.run(input, chl));
    }
    else if (idx == 1)
    {
        RsPsiReceiver recver;

        recver.init(numElements, numElements, 40, ZeroBlock, false, numThreads);

        coproto::sync_wait(recver.run(input, chl));

        out.resize(numElements);

        for (u64 matchIdx : recver.mIntersection) {
            if (matchIdx < numElements) {
                out[matchIdx] = 1;
            }
        }
    }

}

void RPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, Socket &chl, u32 numThreads){
    std::vector<block> OPPRF_Out(numElements);
    StdLog* log = StdLog::instance();

    if(idx == 0){
        Timer timer;
        auto __t0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();
        SoOPPRFRecver recver(numElements, numElements, 1, false, &chl);
        recver.OPPRF(set, OPPRF_Out);
        auto __t1 = timer.setTimePoint("SoOPPRF");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t1 - __t0).count();
        if (log) log->record("SoOPPRF", timeMs, commMB);

        Timer timer2;
        auto __t2 = timer2.setTimePoint("start");
        double commStart2 = chl.bytesSent() + chl.bytesReceived();
        PEQT(0, OPPRF_Out, out, chl, 1);
        auto __t3 = timer2.setTimePoint("PEQT");

        double commMB2 = (chl.bytesSent() + chl.bytesReceived() - commStart2) / 1024.0 / 1024.0;
        double timeMs2 = std::chrono::duration<double, std::milli>(__t3 - __t2).count();
        if (log) log->record("PEQT", timeMs2, commMB2);
    }else if(idx == 1){
        Timer timer;
        auto __t0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();
        SoOPPRFSender sender(numElements, numElements, 1, false, &chl);
        sender.OPPRF(set, OPPRF_Out);
        auto __t1 = timer.setTimePoint("SoOPPRF");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t1 - __t0).count();
        if (log) log->record("SoOPPRF", timeMs, commMB);

        Timer timer2;
        auto __t2 = timer2.setTimePoint("start");
        double commStart2 = chl.bytesSent() + chl.bytesReceived();
        PEQT(1, OPPRF_Out, out, chl, 1);
        auto __t3 = timer2.setTimePoint("PEQT");

        double commMB2 = (chl.bytesSent() + chl.bytesReceived() - commStart2) / 1024.0 / 1024.0;
        double timeMs2 = std::chrono::duration<double, std::milli>(__t3 - __t2).count();
        if (log) log->record("PEQT", timeMs2, commMB2);
    }
}

void PSU(u32 idx, u32 numElements, std::vector<block> set, std::vector<block> &PSU_Out, Socket &chl, u32 numThreads){
    BitVector peqt_out(numElements);

    if(idx == 0){
        RPMT(0, numElements, set, peqt_out, chl, numThreads);
        OT(0, numElements, set, peqt_out, PSU_Out, chl, numThreads);
    }else if(idx == 1){
        StdLog log(idx, std::log2(numElements), "PSU-Fast");
        log.printHeader();

        RPMT(1, numElements, set, peqt_out, chl, numThreads);

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
