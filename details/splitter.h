#pragma once

namespace NHasher {
    class Splitter
        : public NCommon::Module<uint8_t>
    {
        typedef NCommon::Module<float>::Ptr Processor;
        public:
            Splitter(const Config& config, const Processor& left, const Processor& right);
            void Push(const uint8_t* data, size_t len);
        private:
            Config config_;
            size_t sample_size_;
            size_t channel_sample_size_;
            std::vector<float> left_buf_;
            std::vector<float> right_buf_;
            uint32_t max_;
            Processor left_;
            Processor right_;
    };

    Splitter::Splitter(const Config& config, const Processor& left, const Processor& right)
        : config_(config)
        , sample_size_(config.GetChannels() * config.GetBits() / 8)
        , channel_sample_size_(sample_size_ / config.GetChannels())
        , left_buf_(config.GetChunkLength() * channel_sample_size_ * config.GetSampleRate())
        , right_buf_(config.GetChunkLength() * channel_sample_size_ * config.GetSampleRate())
        , max_(0xFFFFFFFF << (32 - config_.GetBits()) >> (32 - config_.GetBits()))
        , left_(left)
        , right_(right)
        
    {
        assert(config.GetChannels() == Config::kStereo);
    }

    void Splitter::Push(const uint8_t* data, size_t len)
    {
        ::printf("Splitter: %lu\n", len);
        assert(len % sample_size_ == 0);
        for (size_t i = 0; i < len - sample_size_; i += sample_size_) {
            uint32_t left = 0;
            for (size_t j = 0; j < channel_sample_size_; ++j) {
                left |= (uint32_t)(*(data + i + j)) << j * 8;
            }

            uint32_t right = 0;
            for (size_t j = 0; j < channel_sample_size_; ++j) {
                right |= (uint32_t)(*(data + i + j + channel_sample_size_)) << j * 8;
            }
            left_buf_[i / 2] = (float)left / max_;
            right_buf_[i / 2] = (float)right / max_;
       }
       left_->Push(&left_buf_[0], len / 2);
       right_->Push(&right_buf_[0], len / 2);
    };
}
