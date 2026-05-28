#include "SoOPPRF.h"

SoOPRFSender::SoOPRFSender(uint64_t num_, uint64_t numThreads_, bool useOle_, coproto::Socket *socket_)
    : num(num_), numThreads(numThreads_), useOle(useOle_), socket(socket_)
{
    sender = new AltModWPrfSender();
    pool = new macoro::thread_pool();
    auto e = pool->make_work();
    pool->create_threads(numThreads);
    sender->mUseMod2F4Ot = !useOle;
    prng = new PRNG(oc::ZeroBlock);

    AltModPrf::KeyType senderKey = AltModPrf::KeyType({
        block(0, 1),
        block(0, 2),
        block(0, 3),
        block(0, 4),
    });

    ole = new CorGenerator();
    ole->init(socket->fork(), *prng, 0, 1, 1 << 18, 0);

    std::vector<oc::block> rk(AltModPrf::KeySize);
    for (u64 i = 0; i < AltModPrf::KeySize; ++i) {
        rk[i] = oc::block(i, *oc::BitIterator((u8 *)&senderKey, i));
    }

    sender->init(num, *ole, AltModPrfKeyMode::SenderOnly, AltModPrfInputMode::ReceiverOnly, senderKey, rk);
}


void SoOPRFSender::OPRF(std::vector<oc::block> &y0)
{
    auto r = coproto::sync_wait(coproto::when_all_ready(ole->start(), sender->evaluate({}, y0, *socket, *prng)));

    std::get<0>(r).result();
    std::get<1>(r).result();
}

SoOPRFSender::~SoOPRFSender()
{
    delete sender;
    delete prng;
    delete ole;
}

SoOPRFRecver::SoOPRFRecver(uint64_t num_, uint64_t numThreads_, bool useOle_, coproto::Socket *socket_)
    : num(num_), numThreads(numThreads_), useOle(useOle_), socket(socket_)
{
    recver = new AltModWPrfReceiver();
    pool = new macoro::thread_pool();
    auto e = pool->make_work();
    pool->create_threads(numThreads);
    recver->mUseMod2F4Ot = !useOle;
    prng = new PRNG(oc::OneBlock);

    ole = new CorGenerator();
    ole->init(socket->fork(), *prng, 1, 1, 1 << 18, 0);


    std::vector<std::array<oc::block, 2>> sk(AltModPrf::KeySize);
    for (u64 i = 0; i < AltModPrf::KeySize; ++i) {
        sk[i][0] = oc::block(i, 0);
        sk[i][1] = oc::block(i, 1);
    }

    recver->init(num, *ole, AltModPrfKeyMode::SenderOnly, AltModPrfInputMode::ReceiverOnly, {}, sk);
};


void SoOPRFRecver::OPRF(std::vector<oc::block> &x, std::vector<oc::block> &y1)
{

    auto r = coproto::sync_wait(coproto::when_all_ready(ole->start(), recver->evaluate(x, y1, *socket, *prng)));

    std::get<0>(r).result();
    std::get<1>(r).result();
}

SoOPRFRecver::~SoOPRFRecver()
{
    delete recver;
    delete prng;
    delete ole;
}

SoOPPRFSender::SoOPPRFSender(uint64_t num_, uint64_t num_kv_, uint64_t numThreads_, bool useOle_, coproto::Socket *socket_)
    : SoOPRFSender(num_, numThreads_, useOle_, socket_)
{
    okvs = new volePSI::Baxos;
    okvs->init(num_kv_, 1 << 14, 3, 40, volePSI::PaxosParam::Binary, oc::ZeroBlock);
}

SoOPPRFSender::~SoOPPRFSender()
{
    delete okvs;
}

void SoOPPRFSender::OPPRF(std::vector<oc::block> &keys, std::vector<oc::block> &y0)
{
    SoOPRFSender::OPRF(y0);
    u32 keyBitLength = 40 + oc::log2ceil(keys.size());
    u64 keyByteLength = oc::divCeil(keyBitLength, 8);   

    AltModPrf prf(SoOPRFSender::getKey());
    std::vector<block> prf_value(keys.size());
    prf.eval(keys, prf_value);


    oc::Matrix<u8> values(keys.size(), keyByteLength);

    for (u64 i = 0; i < prf_value.size(); i++) {
        std::memcpy(values[i].data(), &prf_value[i], keyByteLength);
    }

    PRNG prng(osuCrypto::sysRandomSeed());
    oc::Matrix<u8> encoding(okvs->size(), keyByteLength);
    okvs->solve<u8>(keys, values, encoding, &prng, 1);

    coproto::sync_wait(socket->send(encoding));
}



SoOPPRFRecver::SoOPPRFRecver(uint64_t num_, uint64_t num_kv_, uint64_t numThreads_, bool useOle_, coproto::Socket *socket_)
    : SoOPRFRecver(num_, numThreads_, useOle_, socket_)
{
    okvs = new volePSI::Baxos;
    okvs->init(num_kv_, 1 << 14, 3, 40, volePSI::PaxosParam::Binary, oc::ZeroBlock);
}

SoOPPRFRecver::~SoOPPRFRecver()
{
    delete okvs;
}

void SoOPPRFRecver::OPPRF(std::vector<oc::block> &keys, std::vector<oc::block> &y1)
{
    std::vector<oc::block> tmp(keys.size());
    u32 keyBitLength = 40 + oc::log2ceil(keys.size());
    u64 keyByteLength = oc::divCeil(keyBitLength, 8);

    SoOPRFRecver::OPRF(keys, tmp);

    oc::Matrix<u8> encoding(okvs->size(), keyByteLength);
    

    coproto::sync_wait(socket->recv(encoding));

    oc::Matrix<u8> d(keys.size(), keyByteLength);
    okvs->decode<u8>(keys, d, encoding, 1);

    for (u64 i = 0; i < keys.size(); ++i) {
        y1[i] = tmp[i];
        u8* d_ptr = d.data(i);
        u8* y_ptr = reinterpret_cast<u8*>(&y1[i]);
        for (u64 j = 0; j < keyByteLength; ++j) {
            y_ptr[j] ^= d_ptr[j];
        }
    }

}
