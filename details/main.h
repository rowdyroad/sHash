#pragma once
#include <boost/thread.hpp>
#include <stack>
#include "common.h"

namespace NHasher {
    template<class T>
    class Main
    {
        public:
            Main(int source, const Config& config, boost::shared_ptr<T>& processor);

        private:
            std::unique_ptr<boost::thread> reader_;
            std::unique_ptr<boost::thread> process_;
            Config config_;
            boost::shared_ptr<T>& processor_;
            volatile bool break_;
            typedef std::vector<uint8_t> Buffer;
            Buffer buf_;
            typedef boost::shared_ptr<Buffer> PBuffer;
            std::stack<PBuffer> all_;
            PBuffer current_all_;
            size_t current_pos_;
            NCommon::Mutex mutex_;
            NCommon::Event event_;
            void reading(int fd);
            void processing();
            void modifyCurrent(uint8_t* data, size_t len, bool create = false);
    };

    template<class T>
    Main<T>::Main(int source, const Config& config, boost::shared_ptr<T>& processor)
        : config_(config)
        , processor_(processor)
        , break_(false)
        , buf_(config_.GetChunkLength() * config_.GetChannels() * config_.GetSampleRate() * config_.GetBits() / 8)
        , current_pos_(0)
    {
        reader_.reset(new boost::thread(boost::bind(&Main::reading, this, source)));
        process_.reset(new boost::thread(boost::bind(&Main::processing, this)));
    }

    template<class T>
    void Main<T>::modifyCurrent(uint8_t* data, size_t len, bool create)
    {
        if (!current_all_ || create) {
            current_all_.reset(new Buffer(buf_.size()));
            current_pos_ = 0;
        }
        if (len) {
            assert(current_pos_ + len <= current_all_->size());
            Buffer& current = *current_all_.get();
            ::memcpy(&current[current_pos_], data, len);
            current_pos_ += len;
        }
    }

    template<class T>
    void Main<T>::reading(int fd)
    {
        while (true) {
            int len = ::read(fd, &buf_[0], buf_.size());
            if (len <=0 && errno != EAGAIN) {
                break;
            }

            size_t ltw = std::min(buf_.size() - current_pos_, static_cast<size_t>(len));
            modifyCurrent(&buf_[0], ltw);

            if (current_pos_ == current_all_->size()) {
                {
                    printf("Add chunk\n");
                    NCommon::Guard guard(mutex_);
                    all_.push(current_all_);
                }
                event_.Signal();
                modifyCurrent(&buf_[ltw], len - ltw, true);
            }
        }
        break_ = true;
        event_.SignalBroadcast();
    }

    template<class T>
    void Main<T>::processing()
    {
        while (!break_) {
            event_.Wait();
            printf("Waited: %d\n", all_.size());
            while (all_.size()) {
                PBuffer pbuf;
                {
                    NCommon::Guard guard(mutex_);
                    pbuf = all_.top();
                    all_.pop();
                    printf("Poped\n");
                }
                Buffer& buf = *pbuf;
                printf("Push %p %d\n", &buf[0], buf.size());
                processor_->Push(&buf[0], buf.size());
            }
        }
    }
}
