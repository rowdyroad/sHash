#include "details/common.h"
#include "details/config.h"
#include "details/wave.h"
#include "details/options.h"
#include "details/hasher.h"
#include "details/dumper.h"
#include "details/floater.h"

using namespace NHasher;

int main(int argc, char** argv)
{
    std::string input;
    size_t sample_rate,
           bits,
           channels,
           step;

    Options opts("SHasher Registrator v.0.1", argc, argv,[&input](Options* opts) {
        (*opts)
            .Add<std::string>("input-file,i", input, "input file");
    });
    
    
    Wave<boost::shared_ptr<Floater>> wave(input, [](const Config& config) {
        auto last = NCommon::Create<Dumper>(config, "dump");
        auto hasher = NCommon::Create<Hasher>(config, last);
        return NCommon::Create<Floater>(config, hasher);     
    });
       
    return 0;
};

