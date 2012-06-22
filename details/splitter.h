#pragma once

namespace NHasher {
    template<class T>
    class Splitter
    {
        public:
            Splitter(const Config& config, boost::shared_ptr<T>& left, boost::shared_ptr<T>& right);
            void Push(const uint8_t* data, size_t len);
        private:
            Config config_;
            size_t sample_size_;
            size_t channel_sample_size_;
            std::vector<float> left_buf_;
            std::vector<float> right_buf_;
            boost::shared_ptr<T>& left_;
            boost::shared_ptr<T>& right_;
    };

    template<class T>
    Splitter<T>::Splitter(const Config& config, boost::shared_ptr<T>& left, boost::shared_ptr<T>& right)
        : config_(config)
        , sample_size_(config.GetChannels() * config.GetBits() / 8)
        , channel_sample_size_(sample_size_ / config.GetChannels())
        , left_buf_(config.GetChunkLength() * channel_sample_size_ * config.GetSampleRate())
        , right_buf_(config.GetChunkLength() * channel_sample_size_ * config.GetSampleRate())
        , left_(left)
        , right_(right)
    {
        assert(config.GetChannels() == Config::kStereo);
    }

    template<class T>
    void Splitter<T>::Push(const uint8_t* data, size_t len)
    {
        ::printf("Splitter: %lu\n", len);
        assert(len % sample_size_ == 0);
        uint32_t max = 0xFFFFFFFF << (32 - config_.GetBits()) >> (32 - config_.GetBits());
        printf("%d\n",max);
        for (size_t i = 0; i < len - sample_size_; i += sample_size_) {
            uint32_t left = 0;
            for (size_t j = 0; j < channel_sample_size_; ++j) {
                left |= (uint32_t)(*(data + i + j)) << j * 8;
            }

            uint32_t right = 0;
            for (size_t j = 0; j < channel_sample_size_; ++j) {
                right |= (uint32_t)(*(data + i + j + channel_sample_size_)) << j * 8;
            }
            left_buf_[i / 2] = (float)left / max;
            right_buf_[i / 2] = (float)right / max;
       }
       left_->Push(&left_buf_[0], len / 2);
       right_->Push(&right_buf_[0], len / 2);
    };
}
