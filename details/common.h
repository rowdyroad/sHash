#pragma once
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace NHasher {
namespace NCommon {
    template<typename T>
    boost::shared_ptr<T> Create(T* inst)
    {
        return boost::shared_ptr<T>(inst);
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

    typedef boost::interprocess::interprocess_mutex Mutex;
    typedef boost::interprocess::scoped_lock<Mutex> Guard;

}
}
