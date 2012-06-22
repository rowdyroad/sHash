#include <memory>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>

#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <audiophash.h>
#include <unistd.h>
#include <sys/wait.h>

struct Config
{
    enum Channels {
        kMono = 1,
        kStereo = 2
    };

    enum Bits {
        k8 = 8,
        k16 = 16
    };

    enum SampleRate {
        k8Khz = 8000,
        k11Khz = 11025,
        k22Khz = 22050,
        k44Khz = 44100,
        k48Khz = 48000
    };
    Channels channels;
    Bits bits;
    SampleRate sample_rate;
    size_t chunk_length;
};

template<class T>
class Main
{
    public:
        Main(const std::string& filename, Config& config, boost::shared_ptr<T>& processor)
            : config_(config)
            , processor_(processor)
            , break_(false)
            , buf_(config.chunk_length * config.channels * config.sample_rate * config.bits / 8)
        {
            ::mkfifo(filename.c_str(), 0666);
            int status;
            int outfd[2];
            int infd[2];
            int oldstdin, oldstdout;
            ::pipe(outfd);
            ::pipe(infd);
            oldstdin = ::dup(0);
            oldstdout = ::dup(1);
            ::close(0);
            ::close(1);
            ::dup2(outfd[0], 0);
            ::dup2(infd[1],1);

            pid_t pid = fork();
            assert(pid != -1);
            if (!pid) {
                close(outfd[0]); // Not required for the child
                close(outfd[1]);
                close(infd[0]);
                close(infd[1]);
                std::string format = (config.bits == Config::k8) ? "U8" : "S16_LE";
                if (execl("/usr/bin/arecord", "arecord",
                                "-c", boost::lexical_cast<std::string>(config.channels).c_str(),
                                "-r", boost::lexical_cast<std::string>(config.sample_rate).c_str(),
                                "-f", format.c_str(),
                                "-t", "raw", NULL) == -1) {
                                perror("error");
                }
            } else {
                close(0); // Restore the original std fds of parent
                close(1);
                dup2(oldstdin, 0);
                dup2(oldstdout, 1);
                close(outfd[0]); // These are being used by the child
                close(infd[1]);
                reader_.reset(new boost::thread(boost::bind(&Main::reading, this, infd[0])));
                ::wait(&status);
            }
        }


    private:
        std::unique_ptr<boost::thread> reader_;
        Config& config_;
        boost::shared_ptr<T>& processor_;
        volatile bool break_;
        std::vector<uint8_t> buf_;

        void reading(int fd)
        {
            while (true) {
                ::printf("read:%d\n", fd);
                int len = ::read(fd, &buf_[0], buf_.size());
                printf("len:%d\n",len);
                if (len <=0 && errno != EAGAIN) {
                    break;
                }
                processor_->Push(&buf_[0], len);
            }
        }
};

template<class T>
class Splitter
{
    public:
        Splitter(Config& config, boost::shared_ptr<T>& left, boost::shared_ptr<T>& right)
            : config_(config)
            , sample_size_(config.channels * config.bits / 8)
            , channel_sample_size_(sample_size_ / config.channels)
            , left_buf_(config.chunk_length * channel_sample_size_ * config.sample_rate)
            , right_buf_(config.chunk_length * channel_sample_size_ * config.sample_rate)
            , left_(left)
            , right_(right)
        {
            assert(config.channels == Config::kStereo);
        }
        void Push(const uint8_t* data, size_t len)
        {
            ::printf("Splitter: %d\n", len);
            assert(len % sample_size_ == 0);
            uint32_t max = 0xFFFFFFFF << (32 - config_.bits) >> (32 - config_.bits);
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

    private:
        Config config_;
        size_t sample_size_;
        size_t channel_sample_size_;
        std::vector<float> left_buf_;
        std::vector<float> right_buf_;
        boost::shared_ptr<T>& left_;
        boost::shared_ptr<T>& right_;
};

class Hasher
{
    public:
        Hasher(const Config& config)
            : config_(config)
        {}
        
        void Push(const float* data, size_t len)
        {
            printf("Received:%d\n",len);
            int nbFrames = 0;
            uint32_t* hash = ph_audiohash(const_cast<float*>(data), static_cast<int>(len), config_.sample_rate, nbFrames);
            if (hash) {
                for (size_t i = 0; i < static_cast<size_t>(nbFrames); ++i) {
                    printf("[%d] - %d\n",i, *(hash + i));
                }
                ::free(hash);
            }
        }
    private:
        Config config_;
};


template<typename T>
boost::shared_ptr<T> Create(T* inst)
{
    return boost::shared_ptr<T>(inst);
}


int main()
{
    Config config;
    config.channels = Config::kStereo;
    config.bits = Config::k16;
    config.sample_rate = Config::k44Khz;
    config.chunk_length = 1;
    auto hasher = Create(new Hasher(config));
    auto splitter = Create(new Splitter<Hasher>(config, hasher, hasher));
    auto main = Create(new Main<Splitter<Hasher>>("/tmp/test", config, splitter));
    return 0;
};

