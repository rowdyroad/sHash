#pragma once
#include <iostream>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include "config.h"
namespace NHasher {
namespace po = boost::program_options;

class Options
{
    public:
        typedef boost::function<void(Options*)> InitHandler;
        Options(const std::string& description, int argc, char** argv, InitHandler func)
            : desc_(description)
            , all_has_defaults_(true)
        {
            opts_ = &(desc_.add_options()("help,h", "produce help message"));
            func(this);
            po::store(po::parse_command_line(argc, argv, desc_), vm_);
            po::notify(vm_); 
            if ((argc == 1 && !all_has_defaults_) || vm_.count("help")) {
                Usage();
            }
        }
        
        void Usage() const
        {
           std::cout << desc_ << "\n";
           exit(1);
        }
  
       template<typename T>
       Options& Add(const std::string& name, const std::string& desc)
       {
            const po::value_semantic* v = po::value<T>();
            (*opts_)(name.c_str(), v, desc.c_str());
            all_has_defaults_ = false;
            return *this;
       }
       
       template<typename T>
       Options& Add(const std::string& name, T& val, T def, const std::string& desc)
       {
            const po::value_semantic* v = po::value<T>(&val)->default_value(def);
            (*opts_)(name.c_str(), v, desc.c_str());
            return *this;
       }
       
       template<typename T>
       Options& Add(const std::string& name, T& val, const std::string& desc)
       {
            const po::value_semantic* v = po::value<T>(&val);
            (*opts_)(name.c_str(), v, desc.c_str());
            all_has_defaults_ = false;
            return *this;
       }
       
       bool Exists(const std::string& name) const
       {
            return vm_.count(name);
       }
       
       template<typename T>
       bool Get(const std::string& name, T& value) const
       {
            if (Exists(name)) {
                value = vm_[name].as<T>();
                return true;
            }
            return false;
       }
       
    private:
        po::options_description desc_;
        po::variables_map vm_;
        po::options_description_easy_init* opts_;
        bool all_has_defaults_;

};


}