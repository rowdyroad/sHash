#pragma once
#include <boost/thread.hpp>
#include <stack>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <audiophash.h>
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
               std::vector<uint32_t> data;
               Stamp(const NStamp::HeaderV1& header)
                    : header(header)
                    , data(header.version_header.frames)
               {}
               
            };
            
            void read(const std::string& name);
            void readdir();
            void parse(int fd);
            Config config_;
            const std::string dir_;
            std::unique_ptr<boost::thread> watcher_;
            std::list<Stamp::Ptr> stamps_;
    };
     
    Comparer::Comparer(const Config& config, const std::string& dir)
        : config_(config)
        , dir_(dir)
    {
        readdir();
    }
    
    void Comparer::Push(const uint32_t* data, size_t count)
    {
        int ret = 0;
        for (auto& stamp : stamps_) {
            printf("%d - %d\n", stamp->data.size(), count);
            timespec ts1, ts2;
            clock_gettime(CLOCK_MONOTONIC, &ts1);
            double* z = ph_audio_distance_ber(&stamp->data[0], stamp->data.size(), const_cast<uint32_t*>(data), count, .3, 256, ret);
            clock_gettime(CLOCK_MONOTONIC, &ts2);
            
            ::printf("time: %d\n", (ts2.tv_sec - ts1.tv_sec) * 1000000000 + (ts2.tv_nsec - ts1.tv_nsec));
            if (ret <= 0) { 
                continue;
            }
            
            double max = *z;
            for (int i = 1; i < ret; ++i) {
                max = std::max(max, *(z + i));
            }
            
            printf("max:%f\n", max);
        }
    }
     
    void Comparer::readdir()
    {
        DIR* dir = ::opendir(dir_.c_str());
        if (!dir) {
            throw std::exception();
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
        printf("read:%s\n", name.c_str());
        std::string path(dir_+'/' + name);
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd == -1) {
            perror("Comparer::read");
            return;
        }        
        parse(fd);
        ::close(fd);
    }    
    
    void Comparer::parse(int fd)
    {
        NStamp::HeaderV1 gh;
        int len = ::read(fd, &gh, sizeof(gh));
        if (len ==-1 || static_cast<size_t>(len) < sizeof(gh)) {
            return;
        }
        if (gh.header.sign != NStamp::GlobalHeader::kSign) {
            printf("incorrect sign\n");
            return;
        }
        
        if (gh.header.version != NStamp::HeaderV1::kVersion) {
            printf("unsupported version\n");
            return;
        }
        
        auto stamp = NCommon::Create<Stamp>(gh);
        printf("%d\n", gh.version_header.frames); 
        
        len = ::read(fd, &stamp->data[0], stamp->data.size());
        if (static_cast<size_t>(len) < stamp->data.size()) {
            printf("incorrect file length\n");
            return;
        }        
        stamps_.push_back(stamp);        
    }
}