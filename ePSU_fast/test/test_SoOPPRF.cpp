#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/block.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtSender.h>
#include <macoro/sync_wait.h>
#include <macoro/when_all.h>
#include <secure-join/Defines.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include "SoOPPRF.h"
#include "StdLog.h"


using namespace secJoin;

void test(u32 nn, u32 idx){
    u32 numElements= 1<< nn;
    std::vector<block> keys(numElements);
    std::vector<block> OPPRF_Out(numElements);

    coproto::Socket chl;
    chl = coproto::asioConnect("localhost:" + std::to_string(1313), idx);

    for (u32 i = 0; i < numElements; i++){
        keys[i] = oc::toBlock(0, i + 1 + idx);
    }

    if(idx == 0){
        SoOPPRFRecver recver(numElements, numElements, 1, false, &chl);
        recver.OPPRF(keys, OPPRF_Out);
    }else{
        StdLog log(idx, nn, "SoOPPRF-Fast");
        log.printHeader();

        oc::Timer timer;
        auto __t0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();
        SoOPPRFSender sender(numElements, numElements, 1, false, &chl);
        sender.OPPRF(keys, OPPRF_Out);
        auto __t1 = timer.setTimePoint("SoOPPRF");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t1 - __t0).count();
        log.record("SoOPPRF", timeMs, commMB);

        log.printSummary();
    }

    if(idx == 0){
        coproto::sync_wait(chl.send(OPPRF_Out));

    }else if(idx == 1){

        std::vector<block> OPPRF_Out1(numElements);
        coproto::sync_wait(chl.recv(OPPRF_Out1));
        u32 count=0;
        if(OPPRF_Out1[0]!=OPPRF_Out[0]){
            count++;
        }
        for(u32 i = 1;i<numElements;i++){
            if(OPPRF_Out1[i]==OPPRF_Out[i]){
                count++;
            }
        }
        if(count == numElements){
            std::cout << "SoOPPRF functionality: ALL PASS" << std::endl;
        }else{
            std::cout << "SoOPPRF functionality: FAIL" << std::endl;
        }
    }
    coproto::sync_wait(chl.flush());
    coproto::sync_wait(chl.close());

}


int main(int argc, char **argv)
{
    oc::CLP cmd;
    cmd.parse(argc, argv);
    u32 nn = cmd.getOr("nn", 14);
    u32 idx = cmd.getOr("r", 0);
    test(nn, idx);
    return 0;
}
