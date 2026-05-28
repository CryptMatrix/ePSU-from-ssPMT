#include "OPermute.h"

void permute(std::vector<u32> pi, std::vector<block> &data){
    std::vector<block> res(data.size());
    for (size_t i = 0; i < pi.size(); ++i){
        res[i] = data[pi[i]];
    }
    data.assign(res.begin(), res.end());
}

void senderROT(u32 numOTs, PRNG& prng, BitVector &send0, BitVector &send1, Socket &chl, u32 numThreads){

    send0.resize(numOTs);
    send1.resize(numOTs);

    SilentOtExtSender sender;
    sender.configure(numOTs, 128, numThreads);



    std::vector<std::array<block,2>> sendMsg(numOTs);

    coproto::sync_wait(sender.genBaseOts(prng, chl));

    coproto::sync_wait(sender.silentSend(sendMsg, prng, chl));


    for (u32 i = 0; i < numOTs; i++){
        const unsigned char* bytes = sendMsg[i][0].data();
        bool bit = (bytes[0] >> 0) & 1;
        send0[i] = bit;

        bytes = sendMsg[i][1].data();
        bit = (bytes[0] >> 0) & 1;
        send1[i] = bit;
    }
    BitVector d;
    d.resize(numOTs);

    coproto::sync_wait(chl.recv(d));
    for (u32 i = 0; i < numOTs; i++){
        if(d[i]==1){
            send0[i]^=send1[i];
            send1[i]^=send0[i];
            send0[i]^=send1[i];
        }
    }

}

void receiverROT(u32 numOTs, PRNG& prng, BitVector &bitV, BitVector &recv, Socket &chl, u32 numThreads){
    bitV.resize(numOTs);
    recv.resize(numOTs);
    BitVector bitV0=bitV;
    SilentOtExtReceiver receiver;
    receiver.configure(numOTs, 128, numThreads);


    std::vector<block> recvMsg(numOTs);

    coproto::sync_wait(receiver.genBaseOts(prng, chl));
    coproto::sync_wait(receiver.silentReceive(bitV, recvMsg, prng, chl, OTType::Random));

    for (u32 i = 0; i < numOTs; i++){
        const unsigned char* bytes = recvMsg[i].data();
        bool bit = (bytes[0] >> 0) & 1;
        recv[i] = bit;
        
    }
    BitVector diff=bitV^bitV0;
    coproto::sync_wait(chl.send(diff));
    bitV=bitV0;
}

void OPermute(u32 idx, BitVector &input, BitVector &output, std::vector<u32> &pi, BitVector &getOccupiedMask, Socket &chl, u32 numThreads){
    u32 num                 = input.size();
    u8 nn                   = std::bit_width(input.size()-1);
    u32 layers              = 2*nn-1;
    u32 switch_num_layer    = num/2;
    u32 switch_num          = layers*switch_num_layer;
    BitVector d(switch_num);

    BitVector input0(num); 
    BitVector* pIn = &input;
    BitVector* pOut = &input0;

    if(idx == 0){ 
        u64 realCount = getOccupiedMask.hammingWeight(); 
        u64 realPtr = 0;
        u64 emptyPtr = realCount; 
        pi.resize(num);
        output.resize(num);

        for (u64 i = 0; i < num; ++i) {
            if (getOccupiedMask[i]) pi[realPtr++] = static_cast<u32>(i);
            else pi[emptyPtr++] = static_cast<u32>(i);
        }

        auto splitIt = pi.begin() + realCount;
        std::shuffle(pi.begin(), splitIt, global_built_in_prg2);
        std::shuffle(splitIt, pi.end(), global_built_in_prg2);

        std::vector<int32_t> pi1(pi.begin(), pi.end()); 
        WaksmanNetwork network(pi1);
        auto switches = network.get_waksman_network();
    
        PRNG prng(sysRandomSeed());
        BitVector recv;
        BitVector bitV(switch_num);
        
        for(u32 i = 0; i < layers; i++){
            for(u32 j = 0; j < switch_num_layer; j++){
                bitV[i*switch_num_layer + j] = (switches[i][j] == 1);
            }
        }
    
        receiverROT(switch_num, prng, bitV, recv, chl, numThreads);
        coproto::sync_wait(chl.recv(d));

        u32 loog    = 1;
        u32 start   = 0;
        std::vector<u32> group_size(switch_num_layer);
        std::vector<u32> switch_start(switch_num_layer);

        group_size[0] = num;
        switch_start[0] = 0;

        for(u8 i=0; i < nn-2; i++){
            start = 0;
            u32 iswitchnum = i * switch_num_layer;
            
            for (u32 j=0; j < loog; j++){
                u32 half_groupsize = group_size[j]/2;
                u32 current_switch_start = iswitchnum + switch_start[j];
                
                for (u32 k=0; k < half_groupsize; k++){
                    u32 sw_idx = current_switch_start + k;
                    
                    u8 s = bitV[sw_idx];
                    u8 r = recv[sw_idx];
                    u8 dd = d[sw_idx];
                    
                    u8 val_in1 = (*pIn)[start + k*2];     
                    u8 val_in2 = (*pIn)[start + k*2 + 1]; 

   
                    u8 val_swapped_1 = s ? val_in2 : val_in1;
                    u8 val_swapped_2 = s ? val_in1 : val_in2;

                    (*pOut)[start + k]                  = val_swapped_1 ^ r ^ (s ? dd : 0);
                    (*pOut)[start + k + half_groupsize] = val_swapped_2 ^ r ^ (s ? 0 : dd);
                }
                
                if(group_size[j] % 2 == 1){
                    (*pOut)[start + group_size[j] - 1] = (*pIn)[start + group_size[j] - 1];
                }
                start += group_size[j];
            }

            for(int ite=loog-1; ite >= 0; ite--){
                group_size[(ite<<1)+1] = (group_size[ite]+1)/2;
                group_size[ite<<1] = group_size[ite]/2; 
                switch_start[(ite<<1)+1] = switch_start[ite] + (group_size[ite<<1]/2);
                switch_start[ite<<1] = switch_start[ite];
            }   
                 
            loog = loog << 1;
            std::swap(pIn, pOut);
        } 
        
        start = 0;
        u32 nn_2layer = switch_num_layer * (nn - 2);
        u32 nn_1layer = switch_num_layer * (nn - 1);
        u32 nn_0layer = switch_num_layer * nn;

        for(u32 i=0; i < group_size.size(); i++){
            u8 v0, v1, v2, v3; 
            u32 sw_base = switch_start[i];
            
            if(group_size[i] == 4){
                v0 = (*pIn)[start]; v1 = (*pIn)[start+1]; v2 = (*pIn)[start+2]; v3 = (*pIn)[start+3];
                
                // Layer 1
                u32 idx1 = nn_2layer + sw_base;     u32 idx2 = idx1 + 1;
                u8 s1 = bitV[idx1]; u8 r1 = recv[idx1]; u8 d1 = d[idx1];
                u8 s2 = bitV[idx2]; u8 r2 = recv[idx2]; u8 d2 = d[idx2];

                u8 t0 = (s1 ? v1 : v0) ^ r1 ^ (s1 ? d1 : 0);
                u8 t2 = (s1 ? v0 : v1) ^ r1 ^ (s1 ? 0 : d1);
                
                u8 t1 = (s2 ? v3 : v2) ^ r2 ^ (s2 ? d2 : 0);
                u8 t3 = (s2 ? v2 : v3) ^ r2 ^ (s2 ? 0 : d2);
                v0=t0; v1=t1; v2=t2; v3=t3;

                // Layer 2
                idx1 = nn_1layer + sw_base;     idx2 = idx1 + 1;
                s1 = bitV[idx1]; r1 = recv[idx1]; d1 = d[idx1];
                s2 = bitV[idx2]; r2 = recv[idx2]; d2 = d[idx2];

                t0 = (s1 ? v1 : v0) ^ r1 ^ (s1 ? d1 : 0);
                t1 = (s1 ? v0 : v1) ^ r1 ^ (s1 ? 0 : d1);

                t2 = (s2 ? v3 : v2) ^ r2 ^ (s2 ? d2 : 0);
                t3 = (s2 ? v2 : v3) ^ r2 ^ (s2 ? 0 : d2);
                v0=t0; v1=t1; v2=t2; v3=t3;

                // Layer 3
                idx1 = nn_0layer + sw_base;
                s1 = bitV[idx1]; r1 = recv[idx1]; d1 = d[idx1];
                
                t0 = (s1 ? v2 : v0) ^ r1 ^ (s1 ? d1 : 0);
                t1 = (s1 ? v0 : v2) ^ r1 ^ (s1 ? 0 : d1);
                
                v2 = v1; 
                v0 = t0; v1 = t1; 

                (*pOut)[start] = v0; (*pOut)[start+1] = v1; (*pOut)[start+2] = v2; (*pOut)[start+3] = v3;

            } else if(group_size[i] == 3){
                v0 = (*pIn)[start]; v1 = (*pIn)[start+1]; v2 = (*pIn)[start+2];

                // Layer 1
                u32 idx = nn_2layer + sw_base;
                u8 s = bitV[idx]; u8 r = recv[idx]; u8 dd = d[idx];
                u8 t0 = (s ? v1 : v0) ^ r ^ (s ? dd : 0);
                u8 t1 = (s ? v0 : v1) ^ r ^ (s ? 0 : dd);
                v0=t0; v1=t1; 

                // Layer 2
                idx = nn_1layer + sw_base;
                s = bitV[idx]; r = recv[idx]; dd = d[idx];
                u8 t1_new = (s ? v2 : v1) ^ r ^ (s ? dd : 0);
                u8 t2_new = (s ? v1 : v2) ^ r ^ (s ? 0 : dd);
                v1=t1_new; v2=t2_new; 

                // Layer 3
                idx = nn_0layer + sw_base;
                s = bitV[idx]; r = recv[idx]; dd = d[idx];
                t0 = (s ? v1 : v0) ^ r ^ (s ? dd : 0);
                t1 = (s ? v0 : v1) ^ r ^ (s ? 0 : dd);
                v0=t0; v1=t1; 

                (*pOut)[start] = v0; (*pOut)[start+1] = v1; (*pOut)[start+2] = v2;

            } else if(group_size[i] == 2){
                v0 = (*pIn)[start]; v1 = (*pIn)[start+1];
                u32 idx = nn_1layer + switch_start[i];
                u8 s = bitV[idx]; u8 r = recv[idx]; u8 dd = d[idx];
                
                u8 t0 = (s ? v1 : v0) ^ r ^ (s ? dd : 0);
                u8 t1 = (s ? v0 : v1) ^ r ^ (s ? 0 : dd);
                
                (*pOut)[start] = t0; (*pOut)[start+1] = t1;
            }
            start += group_size[i];
        }
        std::swap(pIn, pOut); 

       
        loog = loog / 2;
        for(u8 i=0; i < nn-2; i++){
            for(int ite=0; ite < loog; ite++){
                group_size[ite] = group_size[ite<<1] + group_size[(ite<<1)+1];
                switch_start[ite] = switch_start[ite<<1];
            }

            start = 0;
            u32 nn_1ilayer = (nn + 1 + i) * switch_num_layer;
            
            for (u32 j=0; j < loog; j++){
                u32 half_groupsize = group_size[j]/2;
                u32 current_switch_start = nn_1ilayer + switch_start[j];

                for (u32 k=0; k < half_groupsize; k++){
                    u32 sw_idx = current_switch_start + k;
                    u8 s = bitV[sw_idx];
                    u8 r = recv[sw_idx];
                    u8 dd = d[sw_idx];
                    
                    u8 val_left = (*pIn)[start + k];
                    u8 val_right = (*pIn)[start + k + half_groupsize];

                    (*pOut)[start + 2*k]     = (s ? val_right : val_left) ^ r ^ (s ? dd : 0);
                    (*pOut)[start + 2*k + 1] = (s ? val_left : val_right) ^ r ^ (s ? 0 : dd);
                }
                if(group_size[j] % 2 == 1){
                    (*pOut)[start + group_size[j] - 1] = (*pIn)[start + group_size[j] - 1];
                }
                start += group_size[j];
            }
            loog = loog / 2;
            std::swap(pIn, pOut); 
        }
        output = *pIn; 

    } else if(idx == 1){ 
        bool isSender = (idx == 1);
        PRNG prng(sysRandomSeed());
        BitVector send0, send1;
        senderROT(switch_num, prng, send0, send1, chl, numThreads);

        u32 loog    = 1;
        u32 start   = 0;
        std::vector<u32> group_size(switch_num_layer);
        std::vector<u32> switch_start(switch_num_layer);
        
        group_size[0] = num;
        switch_start[0] = 0;

        for(u8 i=0; i < nn-2; i++){
            start = 0;
            u32 switch_offset = switch_num_layer * i;
            for (u32 j=0; j < loog; j++){
                u32 half_group = group_size[j]/2;
                u32 current_sw_start = switch_offset + switch_start[j];

                for (u32 k=0; k < half_group; k++){
                    u32 sw_idx = current_sw_start + k;
                    
                    // Read the input values.
                    u8 in_0 = (*pIn)[start + k*2];      
                    u8 in_1 = (*pIn)[start + k*2 + 1]; 
                    
                    u8 s0 = send0[sw_idx];
                    u8 s1 = send1[sw_idx];

                    (*pOut)[start + k]              = in_0 ^ s0;
                    (*pOut)[start + k + half_group] = in_0 ^ s1; 
                    
                    d[sw_idx]                       = in_0 ^ in_1 ^ s0 ^ s1;
                }
                if(group_size[j] % 2 == 1){
                    (*pOut)[start + group_size[j] - 1] = (*pIn)[start + group_size[j] - 1];
                }
                start += group_size[j];
            }
            
            for(int ite=loog-1; ite >= 0; ite--){
                group_size[(ite<<1)+1] = (group_size[ite]+1)/2;
                group_size[ite<<1] = group_size[ite]/2; 
                switch_start[(ite<<1)+1] = switch_start[ite] + (group_size[ite<<1]/2);
                switch_start[ite<<1] = switch_start[ite];
            }          
            loog = loog << 1;
            std::swap(pIn, pOut);
        }   

        start = 0;
        for(u32 i=0; i < group_size.size(); i++){
            u32 nn_2layer = switch_num_layer*(nn-2);
            u32 nn_1layer = switch_num_layer*(nn-1);
            u32 nn_0layer = switch_num_layer*nn;
            
            u8 v0, v1, v2, v3;
            u32 sw_base = switch_start[i];

            if(group_size[i] == 4){
                v0 = (*pIn)[start]; v1 = (*pIn)[start+1]; v2 = (*pIn)[start+2]; v3 = (*pIn)[start+3];
                
                // Layer 1
                u32 idx = nn_2layer + sw_base;
                u8 s0 = send0[idx]; u8 s1 = send1[idx];
                u8 t0 = v0 ^ s0; 
                u8 t1 = v2 ^ send0[idx+1]; 
                u8 t2 = v0 ^ s1;
                u8 t3 = v2 ^ send1[idx+1];
                d[idx] = v0 ^ v1 ^ s0 ^ s1;
                d[idx+1] = v2 ^ v3 ^ send0[idx+1] ^ send1[idx+1];
                v0=t0; v1=t1; v2=t2; v3=t3;

                // Layer 2
                idx = nn_1layer + sw_base;
                t0 = v0 ^ send0[idx];
                t1 = v0 ^ send1[idx];
                d[idx] = v0 ^ v1 ^ send0[idx] ^ send1[idx];
                
                t2 = v2 ^ send0[idx+1];
                t3 = v2 ^ send1[idx+1];
                d[idx+1] = v2 ^ v3 ^ send0[idx+1] ^ send1[idx+1];
                v0=t0; v1=t1; v2=t2; v3=t3;

                // Layer 3
                idx = nn_0layer + sw_base;
                t0 = v0 ^ send0[idx];
                t1 = v0 ^ send1[idx];
                d[idx] = v0 ^ v2 ^ send0[idx] ^ send1[idx]; 
                
                v2 = v1; 
                v0 = t0; v1 = t1;
                
                (*pOut)[start]=v0; (*pOut)[start+1]=v1; (*pOut)[start+2]=v2; (*pOut)[start+3]=v3;

            } else if(group_size[i] == 3){
                v0 = (*pIn)[start]; v1 = (*pIn)[start+1]; v2 = (*pIn)[start+2];
                
                // Layer 1
                u32 idx = nn_2layer + sw_base;
                u8 s0=send0[idx]; u8 s1=send1[idx];
                u8 t0 = v0 ^ s0;
                u8 t1 = v0 ^ s1;
                d[idx] = v0 ^ v1 ^ s0 ^ s1;
                v0=t0; v1=t1; 

                // Layer 2
                idx = nn_1layer + sw_base;
                s0=send0[idx]; s1=send1[idx];
                t1 = v1 ^ s0;
                u8 t2 = v1 ^ s1;
                d[idx] = v1 ^ v2 ^ s0 ^ s1;
                v1=t1; v2=t2; 

                // Layer 3
                idx = nn_0layer + sw_base;
                s0=send0[idx]; s1=send1[idx];
                t0 = v0 ^ s0;
                t1 = v0 ^ s1;
                d[idx] = v0 ^ v1 ^ s0 ^ s1;
                v0=t0; v1=t1; 

                (*pOut)[start]=v0; (*pOut)[start+1]=v1; (*pOut)[start+2]=v2;

            } else if(group_size[i] == 2){
                v0 = (*pIn)[start]; v1 = (*pIn)[start+1];
                u32 idx = nn_1layer + sw_base;
                
                u8 t0 = v0 ^ send0[idx];
                u8 t1 = v0 ^ send1[idx];
                d[idx] = v0 ^ v1 ^ send0[idx] ^ send1[idx];
                
                (*pOut)[start]=t0; (*pOut)[start+1]=t1;
            }
            start += group_size[i];
        }
        std::swap(pIn, pOut);

        loog = loog / 2;
        for(u8 i=0; i < nn-2; i++){
            u32 nn_1iswitchnum = (nn+1+i)*switch_num_layer;
            for(int ite=0; ite < loog; ite++){
                group_size[ite] = group_size[ite<<1] + group_size[(ite<<1)+1];
                switch_start[ite] = switch_start[ite<<1];
            }
            start = 0;
            for (u32 j=0; j < loog; j++){
                u32 half_group = group_size[j]/2;
                u32 current_sw_start = nn_1iswitchnum + switch_start[j];
                
                for (u32 k=0; k < half_group; k++){     
                    u32 sw_idx = current_sw_start + k;
                    u8 in_0 = (*pIn)[start + k]; // input[start+k]
                    u8 in_1 = (*pIn)[start + k + half_group]; // input[start+k+half]
                    
                    u8 s0 = send0[sw_idx];
                    u8 s1 = send1[sw_idx];

                    (*pOut)[start + 2*k]     = in_0 ^ s0;
                    (*pOut)[start + 2*k + 1] = in_0 ^ s1;
                    d[sw_idx]                = in_0 ^ in_1 ^ s0 ^ s1;
                }
                if(group_size[j] % 2 == 1){
                    (*pOut)[start + group_size[j] - 1] = (*pIn)[start + group_size[j] - 1];
                }
                start += group_size[j];
            }
            loog = loog / 2;
            std::swap(pIn, pOut);
        }
        output = *pIn;
        coproto::sync_wait(chl.send(d));
    }
}
