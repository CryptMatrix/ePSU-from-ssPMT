#pragma once

#include <cstdint>
// #include "SoOPRF.h"
#include <array>
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/block.h>
#include <cstdint>
#include <macoro/start_on.h>
#include "secure-join/Defines.h"
#include "secure-join/Prf/AltModPrfProto.h"
#include "volePSI/Paxos.h"

using namespace secJoin;

class SoOPRFSender {
public:
    SoOPRFSender(uint64_t num_, uint64_t numThreads_, bool useOle_, coproto::Socket *socket_);
    ~SoOPRFSender();

    void setup();
    void OPRF(std::vector<oc::block> &y0);

    AltModPrf::KeyType getKey()
    {
        return sender->getKey();
    }

    uint64_t num;
    uint64_t numThreads;
    bool useOle;
    coproto::Socket *socket;
    PRNG *prng;

private:
    AltModWPrfSender *sender;
    macoro::thread_pool *pool;
    CorGenerator *ole;
};

class SoOPRFRecver {
public:
    SoOPRFRecver(uint64_t num_, uint64_t numThreads_, bool useOle_, coproto::Socket *socket_);
    ~SoOPRFRecver();

    void setup();
    void OPRF(std::vector<oc::block> &x, std::vector<oc::block> &y1);

    uint64_t num;
    uint64_t numThreads;
    bool useOle;
    coproto::Socket *socket;
    PRNG *prng;

private:
    AltModWPrfReceiver *recver;
    macoro::thread_pool *pool;
    CorGenerator *ole;
};

class SoOPPRFSender : public SoOPRFSender {
public:
    SoOPPRFSender(uint64_t num_, uint64_t num_kv_, uint64_t numThreads_, bool useOle_, coproto::Socket *socket_);
    ~SoOPPRFSender();

    void OPPRF(std::vector<oc::block> &keys, std::vector<oc::block> &y0);


private:
    volePSI::Baxos *okvs;
};

class SoOPPRFRecver : public SoOPRFRecver {
public:
    SoOPPRFRecver(uint64_t num_, uint64_t num_kv_, uint64_t numThreads_, bool useOle_, coproto::Socket *socket_);
    ~SoOPPRFRecver();

    void OPPRF(std::vector<oc::block> &keys, std::vector<oc::block> &y1);

private:
    volePSI::Baxos *okvs;
};
