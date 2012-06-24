#pragma once
#include <stddef.h>

namespace NHasher {
    class Config
    {
        public:
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


            Channels GetChannels() const
            {
                return channels_;
            }

            Bits GetBits() const
            {
                return bits_;
            }

            SampleRate GetSampleRate() const
            {
                return sample_rate_;
            }

            size_t GetChunkLength() const
            {
                return chunk_length_;
            }
            
            Config(Channels channels, Bits bits, SampleRate sample_rate, size_t chunk_length)
                : channels_(channels)
                , bits_(bits)
                , sample_rate_(sample_rate)
                , chunk_length_(chunk_length)
            {}
            
            Config()
                : channels_(kMono)
                , bits_(k8)
                , sample_rate_(k8Khz)
                , chunk_length_(1)    
            {}

        private:
            Channels channels_;
            Bits bits_;
            SampleRate sample_rate_;
            size_t chunk_length_;
    } __attribute__((packed));
}
