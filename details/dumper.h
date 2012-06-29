#pragma once
#include "config.h"
#include "common.h"
#include "stamp.h"
#include "exceptions.h"

namespace NHasher {

    class Dumper
        : public NCommon::Module<uint32_t>
    {
        public:
            Dumper(const Config& config, const std::string& target, bool is_file = false);
            void Push(const uint32_t* hash, size_t count);
        private:
            const std::string target_;
            Config config_;
            size_t c_;
            bool is_file_;
    };

    Dumper::Dumper(const Config& config, const std::string& target, bool is_file)
        : target_(target)
        , config_(config)
        , c_(0)
        , is_file_(is_file)
    {}

    void Dumper::Push(const uint32_t* hash, size_t count)
    {
        printf("Dumper: %lu\n", count);
        std::string filename;
        if (!is_file_) {
            filename =  target_+
                        "/" +
                        NCommon::MD5Hash(reinterpret_cast<const uint8_t*>(hash), count * sizeof(uint32_t)).HashStr() + ".hsh";
        } else {
            filename = target_;
        }

        ::printf("Write to file:%s\n", filename.c_str());
        int fd = open(filename.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            throw NException::Error("File to dump open error");
        }
        NStamp::HeaderV1 header(config_, count);
        ::write(fd, &header, sizeof(header));
        ::write(fd, hash, sizeof(uint32_t) * count);
        ::close(fd);
    }

}