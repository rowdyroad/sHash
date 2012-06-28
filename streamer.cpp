#include "details/options.h"
#include "details/common.h"
#include "details/config.h"
#include "details/source.h"
#include "details/reader.h"
#include "details/splitter.h"
#include "details/hasher.h"
#include "details/dumper.h"
#include "details/floater.h"
#include "details/comparer.h"

using namespace NHasher;

template<class First, class Second>
void Start(const std::string& filename, const std::string& device, const Config& config, const Second& module)
{
    Arecord<boost::shared_ptr<First>> source(filename, device, config, [&config, &module](int source) {
        return NCommon::Create<First>(source, config, module);
    });
}


int main(int argc, char** argv)
{

    std::string device;
    size_t sample_rate,
           bits,
           channels,
           step;
    Options opts("SHasher v.0.1", argc, argv,[&device, &sample_rate, &bits, &channels, &step](Options* opts) {
        (*opts)
            .Add<std::string>("device,d", device, "default", "arecord pcm device")
            .Add<size_t>("samplerate,s", sample_rate, Config::k8Khz,"samples rate")
            .Add<size_t>("channels,c", channels, Config::kMono, "channels")
            .Add<size_t>("format,f", bits, Config::k8, "bits per sample")
            .Add<size_t>("step,S", step, 1, "bits per sample");
    });
    
    Config config((Config::Channels)channels, (Config::Bits)bits, (Config::SampleRate)sample_rate, 10);
    auto last = NCommon::Create<Comparer>(config, "dump");
    
    auto hasher = NCommon::Create<Hasher>(config, last);
    
    NCommon::Module<uint8_t>::Ptr first;
    
    if (config.GetChannels() == Config::kStereo) {
        first = NCommon::Create<Splitter>(config, hasher, hasher);
    } else {
        first = NCommon::Create<Floater>(config, hasher);
    }
    
    Start<Reader>("/tmp/test","default", config, first);
    

    return 0;
};

