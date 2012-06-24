#pragma once

namespace NHasher {
    class Floater
        : public NCommon::Module<uint8_t>
    {
        typedef NCommon::Module<float>::Ptr Processor;
        public:
        
            Floater(const Config& config, const Processor& processor);
            void Push(const uint8_t* data, size_t len);
        private:
            Config config_;
            uint32_t max_;
            size_t sample_size_;
            std::vector<float> buf_;
            Processor processor_;
    };
    
    Floater::Floater(const Config& config, const Processor& processor)
        : config_(config)
        , max_(0xFFFFFFFF << (32 - config_.GetBits()) >> (32 - config_.GetBits()))
        , sample_size_(config.GetChannels() * config.GetBits() / 8)
        , buf_(config.GetChunkLength() * sample_size_ * config.GetSampleRate())
        , processor_(processor)
    {}
    
    void Floater::Push(const uint8_t* data, size_t len)
    {
       for (size_t i = 0; i < len - sample_size_; i += sample_size_) {
           uint32_t chunk = 0;
           for (size_t j = 0; j < sample_size_; ++j) {
                chunk |= (uint32_t)(*(data + i + j)) << j * 8;
           }
           buf_[i / 2] = (float)chunk / max_;
       }

       processor_->Push(&buf_[0], buf_.size());
    }
}