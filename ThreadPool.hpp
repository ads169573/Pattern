#ifndef _THREADPOOL_HPP_
#define _THREADPOOL_HPP_
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <iostream>
#include <functional>
#include <condition_variable>
#include <future>
#include <memory>

class ThreadPool{
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex lock;
    std::condition_variable cv;
    bool finish = false;
public:
    ThreadPool(size_t num){
        finish = false;
        for(size_t i=0;i<num;++i){
            workers.emplace_back([this]{
                for(;;){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->lock);
                        this->cv.wait(lock, [this]{
                            return (this->finish || (!this->tasks.empty()));
                        });
                        if(this->finish && this->tasks.empty()){
                            break;
                        }
                        
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }
    
    template<class F,class... Args>
    auto enqueue(F&& func,Args&&... args)
        ->std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(func),std::forward<Args>(args)...)
        );
        std::future<return_type> ret = task->get_future();
        {
            std::unique_lock<std::mutex> lock(this->lock);
            if(this->finish){
                throw std::runtime_error("thread pool has already stopped");
            }
            tasks.emplace([task](){
                (*task)();
            });
        }
        
        cv.notify_one();
        return ret;
    }
    
    ~ThreadPool(){
        {
            std::unique_lock<std::mutex> lock(this->lock);
            finish = true;
        }
        
        cv.notify_all();
        
        for(auto &worker:workers){
            worker.join();
        }
        std::cout<<"thread pool end\n";
    }
};

#endif