#pragma once
#include <sndfile.h>
#include <algorithm>

namespace NHasher {
    class Floater
        : public NCommon::Module<uint8_t>
    {
        class SFVirtualWrapper
        {
            const uint8_t* data_;
            size_t size_;
            size_t pos_;
            SNDFILE *fd_;
            SF_VIRTUAL_IO vio_;
            SF_INFO info_;

            public:
                SFVirtualWrapper(const Config& config, const uint8_t* data, size_t size)
                    : data_(data)
                    , size_(size)
                    , pos_(0)
                    , fd_(nullptr)
                {
                    vio_.get_filelen = &SFVirtualWrapper::length;
                    vio_.seek          = &SFVirtualWrapper::seek;
                    vio_.read          = &SFVirtualWrapper::read;
                    vio_.write         = &SFVirtualWrapper::write;
                    vio_.tell          = &SFVirtualWrapper::tell;
                    info_.format = SF_FORMAT_RAW;
                    if (config.GetBits() == 16) {
                        info_.format |= SF_FORMAT_PCM_16;
                    } else {
                        info_.format |= SF_FORMAT_PCM_S8;
                    }
                    info_.samplerate = config.GetSampleRate();
                    info_.channels = 1;
                    info_.seekable = 0;
                    fd_ = sf_open_virtual(&vio_, SFM_READ, &info_, this);
                    if (!fd_) {
                        throw NException::Error("Sound file open error");
                    }
                    sf_command(fd_, SFC_SET_NORM_FLOAT, NULL, SF_TRUE);
                };

                SNDFILE* Fd() const
                {
                    return fd_;
                }

                const SF_INFO& Info() const
                {
                    return info_;
                }

                ~SFVirtualWrapper()
                {
                    if (fd_) {
                        ::sf_close(fd_);
                    }
                }

            static sf_count_t length(void* wrp)
            {
                printf("len\n");
                return ((SFVirtualWrapper*)wrp)->size_;
            }

            static sf_count_t seek(sf_count_t offset, int whence, void* wrp)
            {
                printf("seek offset:%lu from:%d\n", offset, whence);
                SFVirtualWrapper* self = ((SFVirtualWrapper*)wrp);
                switch (whence) {
                  case SEEK_SET:
                    self->pos_ = offset;
                    break;
                  case SEEK_CUR:
                    self->pos_ = self->pos_ + offset;
                    break;
                  case SEEK_END:
                    self->pos_ = self->size_ - offset;
                    break;
                };

                return self->pos_;
            }

            static sf_count_t read(void *ptr, sf_count_t count, void *wrp)
            {
                printf("read count:%lu\n", count);
                SFVirtualWrapper* self = ((SFVirtualWrapper*)wrp);
                if (self->pos_ >= self->size_) {
                    return 0;
                }
                size_t len = std::min(static_cast<size_t>(count), self->size_ - self->pos_);
                ::memcpy(ptr, self->data_ + self->pos_, len);
                self->pos_ += len;
                return len;
            }

            static sf_count_t write(const void *ptr, sf_count_t count, void *wrp)
            {
               printf("write\n");
               return -1;
            }

            static sf_count_t tell(void* wrp)
            {
                printf("tell\n");
                return ((SFVirtualWrapper*)wrp)->pos_;
            }
        };

        typedef NCommon::Module<float>::Ptr Processor;
        public:
            Floater(const Config& config, const Processor& processor);
            void Push(const uint8_t* data, size_t len);
        private:
            Config config_;
            uint32_t max_;
            size_t sample_size_;
            Processor processor_;
    };

    Floater::Floater(const Config& config, const Processor& processor)
        : config_(config)
        , processor_(processor)
    {}

    void Floater::Push(const uint8_t* data, size_t len)
    {
        printf("Floater len:%lu\n", len);
        SFVirtualWrapper wrp(config_, data, len);
        std::vector<float> buf(wrp.Info().frames * wrp.Info().channels);
        sf_count_t ret = sf_readf_float(wrp.Fd(), &buf[0], buf.size());
        if (ret == -1 || (static_cast<size_t>(ret) != buf.size())) {
            throw NException::Error("Sound file read error");
        }
        processor_->Push(&buf[0], buf.size());
    }
}