#pragma once
#include <audiophash.h>
#include <boost/format.hpp>


namespace NHasher {
    class Hasher
    {
        public:
            struct GlobalHeader
            {
                enum  {
                    kSign = 0xAABBCCDD
                };
                uint32_t sign;
                uint8_t version;
                GlobalHeader(uint8_t version)
                    : sign(kSign)
                    , version(version)
                {}
            } __attribute__((packed));

            struct HeaderV1
            {
                enum {
                    kVersion = 1
                };
                GlobalHeader header;
                Config config;
                uint32_t frames;
                HeaderV1(const Config& config, uint32_t frames)
                    : header(1)
                    , config(config)
                    , frames(frames)
                {}
            } __attribute__((packed));

            Hasher(const Config& config);
            void Push(const float* data, size_t len);
        private:
            Config config_;
            int c;
    };

    Hasher::Hasher(const Config& config)
        : config_(config)
        , c(0)
    {}

    void Hasher::Push(const float* data, size_t len)
    {
        printf("Received:%lu\n",len);
        int nbFrames = 0;
        uint32_t* hash = ph_audiohash(const_cast<float*>(data), static_cast<int>(len), config_.GetSampleRate(), nbFrames);
        if (hash) {
            std::string filename((boost::format("dump/%1%") % ++c).str());
            ::printf("Write to file:%s\n", filename.c_str());
            int fd = open(filename.c_str(), O_CREAT | O_RDWR, 0666);
            if (fd == -1) {
                perror("open file error");
            }
            assert(fd != -1);
            HeaderV1 header(config_, static_cast<uint32_t>(nbFrames));
            ::write(fd, &header, sizeof(header));
            ::write(fd, hash, sizeof(uint32_t) * nbFrames);
            ::close(fd);
            ::free(hash);
        }
    }
}
