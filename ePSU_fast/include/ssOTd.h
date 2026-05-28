#pragma once

#include <libOTe/TwoChooseOne/Silent/SilentOtExtSender.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h>
#include <libOTe/Base/BaseOT.h>
#include <coproto/Socket/AsioSocket.h>

#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/Log.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>

using namespace oc;
using namespace std;
using coproto::Socket;

void printBlock(const osuCrypto::block& b);
void silentSend(u32 numOTs, Socket &chl, PRNG& prng, AlignedVector<std::array<block, 2>> &sMsgs, u32 numThreads = 1);
void silentRecv(u32 numOTs, Socket &chl, PRNG& prng, BitVector &choices, AlignedVector<block> &rMsgs, u32 numThreads = 1);
void ssROT(bool isSender,u32 numOTs, Socket &chl, BitVector &choices, std::vector<block> &Out, PRNG& prng, u32 numThreads = 1);
void ssOTd(u32 idx, u32 numElements, std::vector<block> sends, BitVector &input, std::vector<block> &Out, Socket &chl, u32 numThreads = 1);
void OT(u32 idx, u32 numElements, std::vector<block> sends, BitVector choices, std::vector<block> &Out, Socket &chl, u32 numThreads);
