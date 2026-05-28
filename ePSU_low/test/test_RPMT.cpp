#include "../ePSU/RPMT.h"
#include "../ePSU/StdLog.h"

using namespace oc;
using namespace std;

void test(u32 idx, u32 nn){
    u32 numElements = 1 << nn;
    u32 nt = 1;
    std::vector<u32> pi;
    std::vector<block> inputSet(numElements);

    BitVector RPMT_Out;

    Socket chl;
    chl = coproto::asioConnect("localhost:" + std::to_string(PORT + 101), idx);

    u32 equalNum = numElements/2;
    for(u32 i = 0; i < equalNum; ++i){
        inputSet[i] = block(0, i+1);
    }
    for(u32 i = equalNum; i < numElements; ++i){
        inputSet[i] = block(0, idx*numElements +i+1);
    }

    if(idx == 0){
        RPMT(idx, numElements, inputSet, RPMT_Out, pi, chl, nt);
    }else{
        StdLog log(idx, nn, "RPMT-Low");
        log.printHeader();
        RPMT(idx, numElements, inputSet, RPMT_Out, pi, chl, nt);
        log.printSummary();
    }

    if(idx == 1){
        u32 count = 0;
        for(u32 i = 0; i < numElements; i++){
            if(RPMT_Out[i] == 1){
                count++;
            }
        }
        if(count == equalNum){
            cout<<"RPMT functionality: ALL PASS"<<endl;
        }else{
            cout<<"RPMT functionality: FAIL"<<endl;
        }
    }


    coproto::sync_wait(chl.flush());
    coproto::sync_wait(chl.close());
}

int main(int agrc, char** argv){
    CLP cmd;
    cmd.parse(agrc, argv);
    u32 nn = cmd.getOr("nn", 14);
    u32 idx = cmd.getOr("r", 0);

    bool help = cmd.isSet("h");
    if (help){
        std::cout << "protocol: secret sharing private membership test" << std::endl;
        std::cout << "parameters" << std::endl;
        std::cout << "    -nn:          logarithm of the number of elements in each set, default 10" << std::endl;
        std::cout << "    -r:           index of party" << std::endl;
        return 0;
    }

    if ((idx > 1 || idx < 0)){
        std::cout << "wrong idx of party, please use -h to print help information" << std::endl;
        return 0;
    }

    test(idx, nn);
    return 0;
}
