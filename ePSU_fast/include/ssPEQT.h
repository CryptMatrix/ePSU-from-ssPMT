#pragma once

#include <cryptoTools/Common/BitVector.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtSender.h>
#include <vector>
#include <volePSI/Defines.h>

using namespace volePSI;
using namespace osuCrypto;

void ssPEQT(u32 idx, std::vector<block> &input, BitVector &out, Socket &chl, u32 numThreads);