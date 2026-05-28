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

#include "ssPEQT.h"
#include "OPermute.h"


#include <string> 
#include <fstream>
#include <algorithm>

using namespace oc;

void ssPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, std::vector<block> &permutedX0, Socket &chl, u32 numThreads, BitVector &getOccupiedMask);


//psPMT=ssPMT+OPermute
void psPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, std::vector<block> &permutedX0, std::vector<u32> &pi, Socket &chl, u32 numThreads);