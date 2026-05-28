#pragma once


#include <stdint.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cmath>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <tuple> 
#include <iomanip>
#include <functional>
#include <algorithm>
#include <random>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <bitset>
#include <omp.h>
#include <assert.h>
#include <immintrin.h>


inline std::random_device rd;
inline std::mt19937 global_built_in_prg(rd());

inline const size_t NUMBER_OF_THREADS = 8;  

inline const size_t CHECK_BUFFER_SIZE = 1024*8;

