#pragma once
#include "config.h"
#include "common.h"
#include "stamp.h"
namespace NHasher {
    
    class Dumper
        : public NCommon::Module<uint32_t>
    {
        public:
            Dumper(const Config& config, const std::string& directory);
            void Push(const uint32_t* hash, size_t count);
        private:
            const std::string dir_;
            Config config_;
            size_t c_;
    };

    Dumper::Dumper(const Config& config, const std::string& directory)
        : dir_(directory)
        , config_(config)
        , c_(0)
    {}

    void Dumper::Push(const uint32_t* hash, size_t count)
    {
        assert(hash);
        std::string filename("dump/1");
        ::printf("Write to file:%s\n", filename.c_str());
        int fd = open(filename.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            perror("open file error");
        }
        assert(fd != -1);
        NStamp::HeaderV1 header(config_, count);
        ::write(fd, &header, sizeof(header));
        ::write(fd, hash, sizeof(uint32_t) * count);
        ::close(fd);
    }

}