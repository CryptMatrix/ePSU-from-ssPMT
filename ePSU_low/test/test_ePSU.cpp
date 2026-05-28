#include "../ePSU/ePSU.h"

using namespace oc;
using namespace std;


void test(u32 idx, u32 nn){
    u32 numElements = 1 << nn;
    u32 nt = 1;
    std::vector<block> inputSet(numElements);

    std::vector<block> ePSU_Out;

    Socket chl;
    chl = coproto::asioConnect("localhost:" + std::to_string(PORT + 101), idx);

    u32 equalNum = numElements/2;
    for(u32 i = 0; i < equalNum; ++i){
        inputSet[i] = block(0, i+1);
    }
    for(u32 i = equalNum; i < numElements; ++i){
        inputSet[i] = block(0, idx*numElements +i+1);
    }

    ePSU(idx, numElements, inputSet, ePSU_Out, chl, nt);

    if(idx == 1){
        if(ePSU_Out.size() == equalNum){
            std::cout << "ePSU functionality: ALL PASS" << std::endl;
        }
        else{
            std::cout << "ePSU functionality: FAIL" << std::endl;
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
