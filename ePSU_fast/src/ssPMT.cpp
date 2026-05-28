#include "ssPMT.h"
#include "StdLog.h"

void ssPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, Socket &chl){
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

        Timer timer1;
        auto __t2 = timer1.setTimePoint("start");
        double commStart1 = chl.bytesSent() + chl.bytesReceived();
        ssPEQT(1, OPPRF_Out, out, chl, 1);
        auto __t3 = timer1.setTimePoint("ssPEQT");

        double commMB1 = (chl.bytesSent() + chl.bytesReceived() - commStart1) / 1024.0 / 1024.0;
        double timeMs1 = std::chrono::duration<double, std::milli>(__t3 - __t2).count();
        if (log) log->record("ssPEQT", timeMs1, commMB1);

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

        Timer timer1;
        auto __t2 = timer1.setTimePoint("start");
        double commStart1 = chl.bytesSent() + chl.bytesReceived();
        ssPEQT(0, OPPRF_Out, out, chl, 1);
        auto __t3 = timer1.setTimePoint("ssPEQT");

        double commMB1 = (chl.bytesSent() + chl.bytesReceived() - commStart1) / 1024.0 / 1024.0;
        double timeMs1 = std::chrono::duration<double, std::milli>(__t3 - __t2).count();
        if (log) log->record("ssPEQT", timeMs1, commMB1);
    }
}
