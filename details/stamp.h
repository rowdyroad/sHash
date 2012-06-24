#pragma once
namespace NHasher {
namespace NStamp {
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
        
        GlobalHeader() {}
    } __attribute__((packed));

    struct Version1Header
    {    
        Config config;
        uint32_t frames;
        Version1Header(const Config& config, uint32_t frames)
            : config(config)
            , frames(frames)
        {}
        
        Version1Header() {}
        
    } __attribute__((packed));

    struct HeaderV1
    {
        enum {
            kVersion = 1
        };
        GlobalHeader header;
        Version1Header version_header;
        
        HeaderV1(const Config& config, uint32_t frames)
            : header(1)
            , version_header(config, frames)
        {}
        
        HeaderV1() {}
    } __attribute__((packed));   
       
}    
}