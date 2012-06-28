#pragma once
#include "config.h"
extern "C" {
#include <openssl/md5.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
}
#include <boost/function.hpp>
#include <boost/bind.hpp>


namespace NHasher {
template<class T>
class Wave
{
    struct Header
    {
        uint32_t    riff;
        uint32_t    riff_size;
        uint32_t    wave;
    } __attribute__((packed));

    struct Format {
        uint16_t    format;
        uint16_t    channels;
        uint32_t    sample_rate;
        uint32_t    bytes_per_sec;
        uint16_t    block_align;
        uint16_t    bits_per_sample;
    } __attribute__((packed));
                
    struct Chunk
    {
        uint32_t name;
        uint32_t size;
    } __attribute__((packed));
    
    class MappedFile
    {
        public:
            MappedFile(const std::string& filename)
            {
                fd_ = ::open(filename.c_str(), O_RDONLY);
                assert(fd_ != -1);
                struct stat stat_buf;
                assert(!fstat(fd_, &stat_buf));
                data_ = static_cast<uint8_t*>(::mmap(0, stat_buf.st_size,PROT_READ, MAP_PRIVATE, fd_, 0));
                size_ = stat_buf.st_size;
                md5sum();
            }

            uint8_t* Data() const
            {
                return data_;
            }

            size_t Size() const
            {
                return size_;
            }

            ~MappedFile()
            {
                ::munmap(data_, size_);
                ::close(fd_);
            }

            const std::vector<uint8_t>& Hash() const
            {
                return hash_;
            }

            const std::string& HashHex() const
            {
                return hash_hex_;
            }

        private:
            int fd_;
            size_t size_;
            uint8_t* data_;
            std::vector<uint8_t> hash_;
            std::string hash_hex_;

            void md5sum()
            {
                hash_.resize(MD5_DIGEST_LENGTH);
                MD5(data_, size_, &hash_[0]);
                std::stringstream ss;
                for (size_t i = 0 ; i < MD5_DIGEST_LENGTH; ++i) {
                    ss << std::hex << static_cast<int>(hash_[i]);
                }
                hash_hex_ = ss.str();
            }
    };
    
        typedef boost::function<void(const Chunk& chunk, uint8_t* data)> Handler;
    public:       
        typedef boost::function<T (const Config& config)> Factory;
        Wave(const std::string& filename, const Factory& factory)
            : filename_(filename)
            , map_(filename)
            , cursor_(map_.Data())
            , format_(nullptr)
            , factory_(factory)
            , chunk_handlers_({
                                std::make_pair(0x61746164, boost::bind(&Wave::OnData, this, _1, _2)),
                                std::make_pair(0x20746d66, boost::bind(&Wave::OnFormat, this, _1, _2))
                             })

        {
           printf("%p\n", cursor_);
           Header* h = reinterpret_cast<Header*>(cursor_);
    
           if (h->riff != 0x46464952) {
                throw std::exception();
           }
           uint8_t* last = cursor_ + h->riff_size;
           if (h->wave != 0x45564157) {
                throw std::exception();
           }
           cursor_ += sizeof(Header);
           while (true) {
                Chunk* chunk = reinterpret_cast<Chunk*>(cursor_);
                printf("%08X - %lu\n", chunk->name, chunk->size);
                cursor_ += sizeof(Chunk);
                auto i = chunk_handlers_.find(chunk->name);
                if (i != chunk_handlers_.end()) {
                    i->second(*chunk, cursor_);
                }
                cursor_ += chunk->size;
                
                if (cursor_ >= (last + sizeof(Chunk))) {
                    break;
                }
                
           }
    
        }
        
        const std::string& Hex() const
        {
            return map_.HashHex();
        }
        
    private:
        const std::string filename_;
        MappedFile map_;
        uint8_t* cursor_;
        Format* format_;
        Factory factory_;
        std::map<uint32_t, Handler> chunk_handlers_;
        
        void OnFormat(const Chunk& chunk, uint8_t* data)
        {
            if (chunk.size < sizeof(Format)) {
                throw std::exception();
            }
            format_ = reinterpret_cast<Format*>(data);
            if (format_->format != 1) {
                throw std::exception();
            }
        }
        
        void OnData(const Chunk& chunk, uint8_t* data)
        {
            Config config((Config::Channels)format_->channels,            
                          (Config::Bits)format_->bits_per_sample,
                          (Config::SampleRate)format_->sample_rate, 
                          static_cast<size_t>(chunk.size / format_->bytes_per_sec));
  
            T p = factory_(config);
            p->Push(data, chunk.size);
        };
};
};