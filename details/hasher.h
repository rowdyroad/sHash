#pragma once
#include <audiophash.h>
#include <boost/format.hpp>

namespace NHasher {
    class Hasher
        : public NCommon::Module<float>
    {
        typedef NCommon::Module<uint32_t>::Ptr Processor;
        public:
            Hasher(const Config& config, const Processor& processor);
            void Push(const float* data, size_t len);
        private:
            Config config_;
            Processor processor_;
            uint32_t max_;
    };
        
    Hasher::Hasher(const Config& config, const Processor& processor)
        : config_(config)
        , processor_(processor)
    {}

    
    void Hasher::Push(const float* data, size_t len)
    {
        printf("Received:%lu\n",len);
        int nbFrames = 0;
        uint32_t* hash = ph_audiohash(const_cast<float*>(data), static_cast<int>(len), config_.GetSampleRate(), nbFrames);
        if (hash) {
            processor_->Push(hash, static_cast<size_t>(nbFrames));
            ::free(hash);
        }
    }
}
