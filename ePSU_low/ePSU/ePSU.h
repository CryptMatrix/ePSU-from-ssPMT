#include "ssOTd.h"
#include "psPMT.h"

using namespace oc;
using namespace std;

void ePSU(u32 idx, u32 numElements, std::vector<block> &set, std::vector<block> &ePSU_Out, Socket &chl, u32 numThreads);