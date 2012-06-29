#pragma once

extern "C" {
#include <openssl/md5.h>
}
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace NHasher {
namespace NCommon {

    template<typename T, typename ... TArgs>
    boost::shared_ptr<T> Create(TArgs && ... args)
    {
        return boost::shared_ptr<T>(new T(std::forward<TArgs>(args) ...));
    }

    class Event
    {
        public:
            void Signal()
            {
                event_.notify_one();
            }

            void SignalBroadcast()
            {
                event_.notify_all();
            }

            void Wait()
            {
                event_.wait(mutex_);
            }
        private:
            boost::interprocess::interprocess_mutex mutex_;
            boost::condition event_;
    };

    template<typename T>
    class Module
    {
        public:
            typedef boost::shared_ptr<Module<T>> Ptr;
            virtual void Push(const T* data, size_t len) = 0;
    };

    class MD5Hash
    {
        public:
            MD5Hash(const uint8_t* data, size_t size)
                : hash_(MD5_DIGEST_LENGTH)
            {
                ::MD5(data, size, &hash_[0]);
                std::stringstream ss;
                for (size_t i = 0 ; i < MD5_DIGEST_LENGTH; ++i) {
                    ss << std::hex << static_cast<int>(hash_[i]);
                }
                hash_hex_ = ss.str();
            }

            const std::vector<uint8_t>& Hash() const
            {
                return hash_;
            }

            const std::string& HashStr() const
            {
                return hash_hex_;
            }
        private:
            std::vector<uint8_t> hash_;
            std::string hash_hex_;
    };

    typedef boost::interprocess::interprocess_mutex Mutex;
    typedef boost::interprocess::scoped_lock<Mutex> Guard;

}
}
