#pragma once
#include <stack>
extern  "C" {
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
}
#include <audiophash.h>
#include <boost/thread.hpp>
#include "exceptions.h"
#include "common.h"

namespace NHasher {
    class Comparer
        : public NCommon::Module<uint32_t>
    {
        public:
            Comparer(const Config& config, const std::string& dir);
            void Push(const uint32_t* data, size_t count);
        private:
            struct Stamp
            {
               typedef boost::shared_ptr<Stamp> Ptr;
               NStamp::HeaderV1 header;
               std::string name;
               size_t index;
               Stamp(const std::string& name, uint32_t index, const NStamp::HeaderV1& header)
                    : header(header)
                    , name(name)
                    , index(index)
               {}
            };

            void read(const std::string& name);
            void readdir();
            void parse(const std::string& name, int fd);
            Config config_;
            const std::string dir_;
            std::unique_ptr<boost::thread> watcher_;
            std::vector<uint32_t> data_;
            size_t frames_;
            std::list<Stamp::Ptr> stamps_;

            //! \todo replace to inotify
            boost::thread refresh_timer_;
            void refresh();
    };


    Comparer::Comparer(const Config& config, const std::string& dir)
        : config_(config)
        , dir_(dir)
        , data_(10*1024*1024)
        , frames_(0)
    {
        readdir();
    }

    void Comparer::Push(const uint32_t* data, size_t count)
    {
        int ret = 0;
        printf("bytes:%lu count:%lu\n", frames_, count);
        double* z = ph_audio_distance_ber(&data_[0],
                                              frames_,
                                              const_cast<uint32_t*>(data),
                                              count,
                                              .30,
                                              128,
                                              ret);
        printf("count:%d\n", ret);
        if (ret <= 0) {
            return;
        }

        double max = 0;
        size_t max_i = 0;
        for (int i = 0; i < ret; ++i) {
            if (max < z[i]) {
                max_i = i;
                max = z[i];
            }
        }
        ::free(z);

        //! /todo interval map
        for (const auto& stamp: stamps_) {
            if (max_i >= stamp->index && max_i < stamp->index + stamp->header.version_header.frames) {
                printf("%s - %f\n", stamp->name.c_str(), max);
                break;
            }
        }
    }

    void Comparer::readdir()
    {
        DIR* dir = ::opendir(dir_.c_str());
        if (!dir) {
            throw NException::Error("Directory open error");
        }

        while (true) {
            struct dirent* f = ::readdir(dir);
            if (!f) {
                break;
            }

            read(f->d_name);
        };
        ::closedir(dir);
    };

    void Comparer::read(const std::string& name)
    {
        std::string path(dir_+'/' + name);
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd == -1) {
            throw NException::Error("File open error");
        }
        parse(name, fd);
        ::close(fd);
    }

    void Comparer::parse(const std::string& name, int fd)
    {
        NStamp::HeaderV1 gh;
        int len = ::read(fd, &gh, sizeof(gh));
        if (len ==-1 || static_cast<size_t>(len) < sizeof(gh)) {
            throw NException::Error("Header read error");
            return;
        }
        if (gh.header.sign != NStamp::GlobalHeader::kSign) {
            throw NException::Base("Incorrect file sign");
        }

        if (gh.header.version != NStamp::HeaderV1::kVersion) {
            throw NException::Base("Unsupported version");
        }

        auto stamp = NCommon::Create<Stamp>(name, frames_, gh);
        size_t bytes = gh.version_header.frames * sizeof(uint32_t);
        len = ::read(fd, &data_[frames_], bytes);
        if (len == - 1 || static_cast<size_t>(len) < bytes) {
            throw NException::Error("Data read error");
        }

        stamps_.push_back(stamp);
        frames_ += len / sizeof(uint32_t);
    }
}