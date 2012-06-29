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
    std::string input,outfile,outdir;

    Options opts("SHasher Registrator v.0.1", argc, argv,[&input,&outfile,&outdir](Options* opts) {
        (*opts)
            .Add<std::string>("input-file,i", input, "input file")
            .Add<std::string>("output,o", outfile, "output file")
            .Add<std::string>("output-dir,O", outdir, "output dir");
    });
    
    if (!outfile.empty() && !outdir.empty()) {
        throw std::exception();
    }
    
    Wave<boost::shared_ptr<Floater>> wave(input, [&outfile, &outdir](const Config& config) {
        auto last = NCommon::Create<Dumper>(config, outfile + outdir, outdir.empty());
        auto hasher = NCommon::Create<Hasher>(config, last);
        return NCommon::Create<Floater>(config, hasher);     
    });
       
    return 0;
};

