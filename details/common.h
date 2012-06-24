#pragma once
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
    
    typedef boost::interprocess::interprocess_mutex Mutex;
    typedef boost::interprocess::scoped_lock<Mutex> Guard;

}
}
