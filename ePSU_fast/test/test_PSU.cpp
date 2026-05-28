// #include <array>
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
#include "RPMT.h"


using namespace secJoin;

void test(u32 nn, u32 idx){
    u32 numElements= 1<< nn;
    std::vector<block> keys(numElements);
    std::vector<block> PSU_Out;

    Socket chl;
    chl = coproto::asioConnect("localhost:" + std::to_string(1313), idx);

    for (u32 i = 0; i < numElements; i++)
    {
        keys[i] = oc::toBlock(0, i + 1 + idx);
    }

    PSU(idx, numElements, keys, PSU_Out, chl, 1);

    if(idx == 1){
        if(PSU_Out.size()==1){
            std::cout << "PSU functionality: ALL PASS" << std::endl;
        }else{
            std::cout << "PSU functionality: FAIL" << std::endl;
        }
    }
    coproto::sync_wait(chl.flush());
    coproto::sync_wait(chl.close());

}


int main(int argc, char **argv)
{
    CLP cmd;
    cmd.parse(argc, argv);
    u32 nn = cmd.getOr("nn", 14);
    u32 idx = cmd.getOr("r", 0);
    test(nn, idx);
    return 0;
}
