#include "ssPEQT.h"
#include <coproto/Socket/Socket.h>
#include <coproto/coproto.h>
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <sys/types.h>
#include <vector>
#include <volePSI/Defines.h>
#include <volePSI/GMW/Circuit.h>
#include <volePSI/GMW/Gmw.h>
#include <volePSI/Paxos.h>
#include <volePSI/config.h>

void ssPEQT(u32 idx, std::vector<block> &input, BitVector &out, Socket &chl, u32 numThreads)
{
    u32 numBins = input.size();
    u64 keyBitLength = 40 + oc::log2ceil(numBins);
    u64 keyByteLength = oc::divCeil(keyBitLength, 8);
    PRNG prng(sysRandomSeed());

    oc::Matrix<u8> mLabel(numBins, keyByteLength);
    for (u32 i = 0; i < numBins; ++i) {
        memcpy(&mLabel(i, 0), &input[i], keyByteLength);
    }

    auto cir = volePSI::isZeroCircuit(keyBitLength);

    volePSI::Gmw cmp;
    cmp.init(mLabel.rows(), cir, numThreads, idx, prng.get());

    if (idx == 1) {
        cmp.setInput(0, mLabel);
    } else {
        cmp.implSetInput(0, mLabel, mLabel.cols());
    }

    coproto::sync_wait(cmp.run(chl));

    oc::Matrix<u8> mOut;
    mOut.resize(numBins, 1);
    cmp.getOutput(0, mOut);

    out.resize(numBins);
    for (u32 i = 0; i < numBins; ++i) {
        out[i] = mOut(i, 0) & 1;
    }
    return;
}