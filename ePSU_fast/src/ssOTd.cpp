#include "ssOTd.h"

void printBlock(const osuCrypto::block& b){
    const unsigned char* bytes = b.data();  
    std::cout << "block[";
    for (int i = 0; i < 16; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)(unsigned char)bytes[i];
        if (i < 15) std::cout << " ";  
    }
    std::cout << "]" << std::dec << std::endl;  
}
void silentSend(u32 numOTs, Socket &chl, PRNG& prng, AlignedVector<std::array<block, 2>> &sMsgs, u32 numThreads ){
    SilentOtExtSender sender;
    sender.configure(numOTs, 128, numThreads);

    coproto::sync_wait(sender.genBaseOts(prng, chl));


    coproto::sync_wait(sender.silentSend(sMsgs, prng, chl));
}
void silentRecv(u32 numOTs, Socket &chl, PRNG& prng, BitVector &choices, AlignedVector<block> &rMsgs, u32 numThreads ){
    SilentOtExtReceiver receiver;
    receiver.configure(numOTs, 128, numThreads);


    coproto::sync_wait(receiver.genBaseOts(prng, chl));

    coproto::sync_wait(receiver.silentReceive(choices, rMsgs, prng, chl, OTType::Random));

}
void ssROT(bool isSender,u32 numOTs, Socket &chl, BitVector &choices, std::vector<block> &Out, PRNG prng, u32 numThreads ){
    Out.resize(numOTs);
    choices.resize(numOTs);

    if(isSender){
        AlignedVector<std::array<block, 2>> sMsgs(numOTs);
        BitVector choices1;
        choices1.resize(numOTs);
        
        silentSend(numOTs, chl, prng, sMsgs, numThreads);

        try{
            coproto::sync_wait(chl.recv(choices1));
        }catch (const std::system_error& e) {
            std::cerr << "send failed: " << e.what() << " (code: " << e.code() << ")" << std::endl;
            return;
        }
        
        for(u32 i = 0; i < numOTs; ++i){
            Out[i] = sMsgs[i][choices[i]^choices1[i]];
            choices[i] = choices[i]^choices1[i];
        }

          
    }else{
        AlignedVector<block> rMsgs(numOTs);
        BitVector choices1=choices;

        
        silentRecv(numOTs, chl, prng, choices, rMsgs, numThreads);

        choices1 ^= choices;
        coproto::sync_wait(chl.send(choices1));
        
        memcpy(Out.data(), rMsgs.data(), numOTs * sizeof(block));

        
    }
}
void ssOTd(u32 idx, u32 numElements, std::vector<block> sends, BitVector &choices, std::vector<block> &Out, Socket &chl, u32 numThreads ){ 
    std::vector<block> OToutputs(numElements);
    Out.reserve(numElements);
    std::vector<block> vecOTP_out(numElements);

    if(idx == 0){
        ssROT(true, numElements, chl, choices, OToutputs, PRNG(osuCrypto::toBlock(123456)), numThreads);
        for(u32 i = 0; i < numElements; i++){
            vecOTP_out[i] = OToutputs[i] ^ sends[i];
        }
        coproto::sync_wait(chl.send(vecOTP_out));
        coproto::sync_wait(chl.send(choices));


    }else if(idx == 1){
        BitVector choices1(numElements);
        std::vector<block> setUnion;
        ssROT(false, numElements, chl, choices, OToutputs, PRNG(osuCrypto::toBlock(654321)), numThreads);
        coproto::sync_wait(chl.recv(vecOTP_out));
        coproto::sync_wait(chl.recv(choices1));
        for(u32 i = 0; i < numElements; ++i){
            if((choices[i]^choices1[i]) == 0){
                vecOTP_out[i] ^= OToutputs[i];
                Out.emplace_back(vecOTP_out[i]);
            }
        }
    }
}
void OT(u32 idx, u32 numElements, std::vector<block> sends, BitVector choices, std::vector<block> &Out, Socket &chl, u32 numThreads){


    if(idx == 0){
        AlignedVector<std::array<block, 2>> sMsgs(numElements);
        std::vector<block> sends1(numElements);
        BitVector choices1(numElements);
        osuCrypto::PRNG prng(osuCrypto::toBlock(123456));
        silentSend(numElements, chl, prng, sMsgs, numThreads);

        coproto::sync_wait(chl.recv(choices1));

        for(u32 i = 0; i < numElements; i++){
            sends1[i] = sMsgs[i][choices1[i]] ^ sends[i];
        }
        coproto::sync_wait(chl.send(sends1));

    }else if(idx == 1){
        AlignedVector<block> rMsgs(numElements);
        std::vector<block> sends1(numElements);
        BitVector choices1(numElements); 
        osuCrypto::PRNG prng(osuCrypto::toBlock(123456));
        silentRecv(numElements, chl, prng, choices1, rMsgs, numThreads);

        choices1 ^= choices;
        coproto::sync_wait(chl.send(choices1));

        Out.reserve(numElements);
        coproto::sync_wait(chl.recv(sends1));
        for (size_t i = 0; i < numElements; i++)
        {
            if((sends1[i]^rMsgs[i]).mData[1] == 0){
                Out.emplace_back(sends1[i]^rMsgs[i]);
            }
        }
        
    }
}