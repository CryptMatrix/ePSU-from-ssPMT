#pragma once

#include <volePSI/Defines.h>
#include <volePSI/config.h>
#include <volePSI/Paxos.h>
#include <volePSI/SimpleIndex.h>

#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/CuckooIndex.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/BitVector.h>
#include <coproto/Socket/AsioSocket.h>   
#include <libOTe/Vole/Silent/SilentVoleReceiver.h>
#include <libOTe/Vole/Silent/SilentVoleSender.h>

#include "SoOPPRF.h"
#include "ssPEQT.h"

using namespace volePSI;
using namespace osuCrypto;


void ssPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, Socket &chl);