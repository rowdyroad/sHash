#pragma once
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
extern "C" {
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
}
#include "config.h"
namespace NHasher {
    namespace NSource
    {
        typedef boost::function<void(int fd)> Handler;

        void FromArecord(const std::string& filename, const Config& config, Handler handler)
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
                std::string format = (config.GetBits() == Config::k8) ? "U8" : "S16_LE";
                if (execl("/usr/bin/arecord", "arecord",
                                "-c", boost::lexical_cast<std::string>(config.GetChannels()).c_str(),
                                "-r", boost::lexical_cast<std::string>(config.GetSampleRate()).c_str(),
                                "-f", format.c_str(),
                                "-t", "raw", NULL) == -1) {
                                perror("error");
                }
            } else {
                close(0);
                close(1);
                dup2(oldstdin, 0);
                dup2(oldstdout, 1);
                close(outfd[0]);
                close(infd[1]);
                handler(infd[0]);
                ::wait(&status);
            }
        }
    }
}
