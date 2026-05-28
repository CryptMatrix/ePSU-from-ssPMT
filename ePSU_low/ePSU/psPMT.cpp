#include "psPMT.h"
#include "StdLog.h"

void ssPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, std::vector<block> &permutedX0, Socket &chl, u32 numThreads, BitVector &getOccupiedMask){
    StdLog* log = StdLog::instance();

    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();
    u32 keyBitLength = ssp + oc::log2ceil(numBins);
    u64 keyByteLength = oc::divCeil(keyBitLength, 8);
    out.resize(numBins);
    permutedX0.resize(numBins);
    block cuckooSeed = block(0x235677879795a931, 0x784915879d3e658a);


    PRNG prng(sysRandomSeed());

    Baxos mPaxos;
    mPaxos.init(3 * numElements, binSize, 3, ssp, volePSI::PaxosParam::Binary, block(0,0));
    u32 okvs_size = mPaxos.size();

    std::vector<block> t_lable(numBins);
    std::vector<block> s_lable(numBins);
    Matrix<u8> P(okvs_size, keyByteLength);
    if(idx == 0){
        Timer timer;
        auto __t_0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();

        oc::CuckooIndex cuckoo;
        cuckoo.init(numElements, ssp, 0, 3);
        cuckoo.insert(set, cuckooSeed);

        getOccupiedMask.resize(cuckoo.mBins.size());
        for (u64 i = 0; i < cuckoo.mBins.size(); ++i) {
            if (!cuckoo.mBins[i].isEmpty()) {
                getOccupiedMask[i] = 1;
            } else {
                getOccupiedMask[i] = 0;
            }
        }

        oc::SilentVoleReceiver<block, block, oc::CoeffCtxGF128> mVoleRecver;
        mVoleRecver.mSecurityType = SilentSecType::SemiHonest;
        mVoleRecver.configure(numBins);
        AlignedUnVector<block> mA(numBins);
        AlignedUnVector<block> mC(numBins);
        coproto::sync_wait(mVoleRecver.silentReceive(mC, mA, prng, chl));

        oc::AES hasher;
        hasher.setKey(cuckooSeed);

        std::vector<block> diffC(numBins);
        std::vector<block> keys(numBins);
        Matrix<u8>values(numBins, keyByteLength);
        permutedX0.resize(numBins);

        for (u32 i = 0; i < numBins; ++i)
        {
            auto bin = cuckoo.mBins[i];

            if (bin.isEmpty() == false)
            {
                auto j = bin.hashIdx();
                auto b = bin.idx();
                block xj = block(set[b].mData[0], j);
                keys[i] = xj;

                permutedX0[i] = block(set[b].mData[0], 1);
                diffC[i] = xj ^ mC[i];
            }
            else
            {
                keys[i] = prng.get();
                diffC[i] = mC[i];
            }

        }

        coproto::sync_wait(chl.send(diffC));

        coproto::sync_wait(chl.recv(P));
        mPaxos.decode<u8>(keys, values, P, 1);


        for (u32 i = 0; i < numBins; ++i)
        {

            block h_mA = hasher.hashBlock(mA[i]);
            block val = block(0, 0);
            std::memcpy(&val, values.data(i), keyByteLength);
            s_lable[i] = h_mA ^ val;
        }

        auto __t_1 = timer.setTimePoint("OPPRF");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t_1 - __t_0).count();
        if (log) log->record("OPPRF", timeMs, commMB);


        Timer timer1;
        auto __t_2 = timer1.setTimePoint("start");
        double commStart1 = chl.bytesSent() + chl.bytesReceived();
        ssPEQT(idx, s_lable, out, chl, numThreads);
        auto __t_3 = timer1.setTimePoint("ssPEQT");

        double commMB1 = (chl.bytesSent() + chl.bytesReceived() - commStart1) / 1024.0 / 1024.0;
        double timeMs1 = std::chrono::duration<double, std::milli>(__t_3 - __t_2).count();
        if (log) log->record("ssPEQT", timeMs1, commMB1);

    }else if(idx == 1){
        Timer timer;
        auto __t_0 = timer.setTimePoint("start");
        double commStart = chl.bytesSent() + chl.bytesReceived();

        volePSI::SimpleIndex sIdx;
        sIdx.init(numBins, numElements, ssp, 3);
        sIdx.insertItems(set, cuckooSeed);

        block mD = prng.get();
        oc::SilentVoleSender<block,block, oc::CoeffCtxGF128> mVoleSender;
        mVoleSender.mSecurityType = SilentSecType::SemiHonest;
        mVoleSender.configure(numBins);
        AlignedUnVector<block> mB(numBins);
        coproto::sync_wait(mVoleSender.silentSend(mD, mB, prng, chl));

        std::vector<block> diffC(numBins);
        coproto::sync_wait(chl.recv(diffC));

        oc::AES hasher;
        hasher.setKey(cuckooSeed);

        std::vector<block> keys(numElements * 3);
        Matrix<u8> values(numElements * 3, keyByteLength);
        u32 countV = 0;
        prng.get(t_lable.data(), numBins);
        block res;

        for (u32 i = 0; i < numBins; ++i)
        {
            auto bin = sIdx.mBins[i];
            auto size = sIdx.mBinSizes[i];

            for (u32 p = 0; p < size; ++p)
            {
                auto j = bin[p].hashIdx();
                auto b = bin[p].idx();

                block yj = block(set[b].mData[0], j);
                keys[countV] = yj;

                yj ^= diffC[i];
                auto tmp = mB[i] ^ (yj.gf128Mul(mD));
                tmp = hasher.hashBlock(tmp);

                res = tmp ^ t_lable[i];
                memcpy(values[countV].data(), &res, keyByteLength);
                countV += 1;
            }
        }

        mPaxos.solve<u8>(keys, values, P, nullptr, 1);
        coproto::sync_wait(chl.send(P));

        auto __t_1 = timer.setTimePoint("OPPRF");

        double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
        double timeMs = std::chrono::duration<double, std::milli>(__t_1 - __t_0).count();
        if (log) log->record("OPPRF", timeMs, commMB);


        Timer timer1;
        auto __t_2 = timer1.setTimePoint("start");
        double commStart1 = chl.bytesSent() + chl.bytesReceived();
        ssPEQT(idx, t_lable, out, chl, numThreads);
        auto __t_3 = timer1.setTimePoint("ssPEQT");

        double commMB1 = (chl.bytesSent() + chl.bytesReceived() - commStart1) / 1024.0 / 1024.0;
        double timeMs1 = std::chrono::duration<double, std::milli>(__t_3 - __t_2).count();
        if (log) log->record("ssPEQT", timeMs1, commMB1);

    }
}


void psPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, std::vector<block> &permutedX0, std::vector<u32> &pi, Socket &chl, u32 numThreads){
    BitVector getOccupiedMask;
    StdLog* log = StdLog::instance();

    ssPMT(idx, numElements, set, out, permutedX0, chl, numThreads, getOccupiedMask);

    Timer timer;
    auto __t_0 = timer.setTimePoint("start");
    double commStart = chl.bytesSent() + chl.bytesReceived();
    OPermute(idx, out, out, pi, getOccupiedMask, chl, numThreads);
    auto __t_1 = timer.setTimePoint("OPermute");

    double commMB = (chl.bytesSent() + chl.bytesReceived() - commStart) / 1024.0 / 1024.0;
    double timeMs = std::chrono::duration<double, std::milli>(__t_1 - __t_0).count();
    if (log) log->record("OPermute", timeMs, commMB);


    permute(pi, permutedX0);
    permutedX0.resize(numElements);
    out.resize(numElements);
}
