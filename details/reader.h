#pragma once
#include <boost/thread.hpp>
#include <stack>
#include "common.h"

namespace NHasher {
    class Reader
    {
        typedef NCommon::Module<uint8_t>::Ptr Processor;
        public:
            Reader(int source, const Config& config, const Processor& processor);

        private:
            std::unique_ptr<boost::thread> reader_;
            std::unique_ptr<boost::thread> process_;
            Config config_;
            Processor processor_;
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

    Reader::Reader(int source, const Config& config, const Processor& processor)
        : config_(config)
        , processor_(processor)
        , break_(false)
        , buf_(config_.GetChunkLength() * config_.GetChannels() * config_.GetSampleRate() * config_.GetBits() / 8)
        , current_pos_(0)
    {
        reader_.reset(new boost::thread(boost::bind(&Reader::reading, this, source)));
        process_.reset(new boost::thread(boost::bind(&Reader::processing, this)));
    }

    void Reader::modifyCurrent(uint8_t* data, size_t len, bool create)
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

    void Reader::reading(int fd)
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

    void Reader::processing()
    {
        while (!break_) {
            event_.Wait();
            printf("Waited: %lu\n", all_.size());
            while (all_.size()) {
                PBuffer pbuf;
                {
                    NCommon::Guard guard(mutex_);
                    pbuf = all_.top();
                    all_.pop();
                }
                Buffer& buf = *pbuf;
                processor_->Push(&buf[0], buf.size());
            }
        }
    }
}
