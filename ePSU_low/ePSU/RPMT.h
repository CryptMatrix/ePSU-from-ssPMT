#pragma once

#include "psPMT.h"
#include "ssOTd.h"



void RPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, std::vector<u32> &pi, Socket &chl, u32 numThreads);

void PSU(u32 idx, u32 numElements, std::vector<block> set, std::vector<block> &PSU_Out, Socket &chl, u32 numThreads);
