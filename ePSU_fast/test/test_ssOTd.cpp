#include "ssOTd.h"
#include "StdLog.h"
#include <cryptoTools/Common/CLP.h>

using namespace oc;
using namespace std;




void test(u32 nn, u32 idx){
    u32 numElements= 1<< nn;
    std::vector<block> inputSet(numElements);
    std::vector<block> ssOTd_Out;
    BitVector choices(numElements);

    Socket chl;
    chl = coproto::asioConnect("localhost:" + std::to_string(1313), idx);

    u32 equalNum = numElements/2;
    for(u32 i = 0; i < numElements; i++){
        inputSet[i] = block(0, i+1);
    }

    for(u32 i = 0; i < equalNum; i++){
        choices[i] = 0;
    }

    if(idx == 0){
        for(u32 i = equalNum; i < numElements; i++){
            choices[i] = 0;
        }
    }else if(idx == 1){
        for(u32 i = equalNum; i < numElements; i++){
            choices[i] = 1;
        }
    }

    if(idx == 0){
        ssOTd(idx, numElements, inputSet, choices, ssOTd_Out, chl, 1);
    }else{
        StdLog log(idx, nn, "ssOTd-Fast");
        log.printHeader();

        Timer timer;
        auto __t0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();

        ssOTd(idx, numElements, inputSet, choices, ssOTd_Out, chl, 1);

        auto __t1 = timer.setTimePoint("ssOTd");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t1 - __t0).count();
        log.record("ssOTd", timeMs, commMB);

        log.printSummary();
    }

    if(idx == 1){
        if(ssOTd_Out.size() == equalNum){
            cout<<"ssOTd functionality: ALL PASS"<<endl;
        }else{
            cout<<"ssOTd functionality: FAIL"<<endl;
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
