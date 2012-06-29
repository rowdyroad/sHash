#pragma once
#include <boost/thread.hpp>
#include <stack>
extern "C" {
#include <sys/epoll.h>
#include <sys/ioctl.h>
}
#include "common.h"
#include "exceptions.h"

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
            typedef boost::shared_ptr<Buffer> PBuffer;
            const size_t size_;
            volatile size_t pos_;
            PBuffer buf_;
            std::stack<PBuffer> queue_;
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
        , size_(config_.GetChunkLength() * config_.GetChannels() * config_.GetSampleRate() * config_.GetBits() / 8)
        , pos_(0)
        , buf_(new Buffer(size_))
    {
        reader_.reset(new boost::thread(boost::bind(&Reader::reading, this, source)));
        process_.reset(new boost::thread(boost::bind(&Reader::processing, this)));
    }

    void Reader::reading(int fd)
    {
        int ep = ::epoll_create(10);
        if (ep == -1) {
            throw NException::Error("Epoll create error");
        }

        struct epoll_event ev, events[10];
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev) == -1) {
            throw NException::Error("Epoll event add error");
        }

        while (true) {
            int ret = ::epoll_wait(ep, events, 10, -1);
            if (ret == -1) {
                break;
            }

            for (int i = 0; i < ret; ++i) {
                if (events[i].data.fd == fd) {
                    int bytes = 0;
                    if (::ioctl(fd, FIONREAD, &bytes) == -1) {
                        throw NException::Error("Get filesize error");
                    }

                    if (static_cast<size_t>(bytes) >= size_) {
                        Buffer& buf = *buf_;
                        ::read(fd, &buf[0], size_);
                        {
                            NCommon::Guard g(mutex_);
                            queue_.push(buf_);
                        }
                        buf_.reset(new Buffer(size_));
                        event_.Signal();
                    }
                }
            }
        }
        break_ = true;
        event_.SignalBroadcast();
    }

    void Reader::processing()
    {
        while (!break_) {
            event_.Wait();
            while (!queue_.empty()) {
                Buffer& buf = *queue_.top();
                processor_->Push(&buf[0], size_);
                {
                    NCommon::Guard g(mutex_);
                    queue_.pop();
                }
            }
        }
    }
}
