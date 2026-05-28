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
#include "ssPEQT.h"
#include "ssPMT.h"
#include "StdLog.h"
using namespace secJoin;


void test(u32 nn, u32 idx){
    u32 numElements= 1<< nn;
    Socket chl;
    chl = coproto::asioConnect("localhost:" + std::to_string(1313), idx);


    std::vector<oc::block> keys(numElements);

    BitVector out(numElements);

    for (u32 i = 0; i < numElements; i++)
    {
        keys[i] = oc::toBlock(0, i + 1 + idx);
    }

    if(idx == 0){
        ssPMT(idx, numElements, keys, out, chl);
    }else{
        StdLog log(idx, nn, "ssPMT-Fast");
        log.printHeader();
        ssPMT(idx, numElements, keys, out, chl);
        log.printSummary();
    }

    if(idx == 0){
        coproto::sync_wait(chl.send(out));
    }else if (idx == 1){
        u32 count = 0;
        BitVector out1(numElements);
        coproto::sync_wait(chl.recv(out1));
        for(u32 i =0;i<numElements;i++){
            if((out[i]^out1[i]) == 0){
                count++;
            }
        }
        if(count == 1){
            std::cout << "ssPMT functionality: ALL PASS" << std::endl;
        }else{
            std::cout << "ssPMT functionality: FAIL" << std::endl;
        }
    }

    coproto::sync_wait(chl.flush());
    coproto::sync_wait(chl.close());



}

int main(int argc, char **argv)
{
    CLP cmd;
    cmd.parse(argc, argv);
    u32 nn = cmd.getOr("nn", 20);
    u32 idx = cmd.getOr("r", 0);
    test(nn, idx);
    return 0;
}
