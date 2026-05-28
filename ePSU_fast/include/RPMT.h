#include <volePSI/RsPsi.h>
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Network/Session.h>
#include <coproto/Socket/AsioSocket.h>
#include <vector>
#include "SoOPPRF.h"
#include "ssOTd.h"



using namespace volePSI;
using namespace oc;

void PEQT(u32 idx, std::vector<block>& input, BitVector& out, Socket& chl, u32 numThreads);

void RPMT(u32 idx, u32 numElements, std::vector<block> set, BitVector &out, Socket &chl, u32 numThreads);

void PSU(u32 idx, u32 numElements, std::vector<block> set, std::vector<block> &PSU_Out, Socket &chl, u32 numThreads);
