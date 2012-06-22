
#include "details/source.h"
#include "details/config.h"
#include "details/main.h"
#include "details/splitter.h"
#include "details/hasher.h"
#include "details/common.h"

int main()
{
    using namespace NHasher;
    Config config(Config::kStereo, Config::k16, Config::k8Khz, 10);
    auto hasher = NCommon::Create(new Hasher(config));
    auto splitter =  NCommon::Create(new Splitter<Hasher>(config, hasher, hasher));

    boost::shared_ptr<Main<Splitter<Hasher>>> main;
    NSource::FromArecord("/tmp/test", config, [&main, &config, &splitter](int source) {
        main = NCommon::Create(new Main<Splitter<Hasher>>(source, config, splitter));
    });
    return 0;
};

