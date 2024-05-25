#ifndef _PIPELINE_HPP_
#define _PIPELINE_HPP_

#include "ThreadPool.hpp"
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <future>
#include <condition_variable>
#include <vector>
#include <memory>
#include <thread>
#include <stdexcept>
#include <functional>
#include <any>
class PipeLine {
private:
    std::vector<std::shared_ptr<ThreadPool>> ThreadPools;
    std::unordered_map<size_t, std::any> result;
    std::unordered_map<size_t, size_t> process;
    std::unordered_set<size_t> check_list;
    size_t stage_num;
    std::mutex lock;
    std::condition_variable cv;
    bool finish = false;

public:
    PipeLine(size_t stage_num, std::vector<size_t> thread_num) {
        this->stage_num = stage_num;
        for (size_t i = 0; i < stage_num; ++i) {
            ThreadPools.emplace_back(std::make_shared<ThreadPool>(thread_num[i]));
        }
    }
    template<class F,class... Args>
    void run(size_t id, F&& func, Args&&... args){
        {
            std::unique_lock<std::mutex> lock(this->lock);
            if(check_list.find(id)==check_list.end()){
                if(finish){
                    throw std::runtime_error("pipeline has already end");
                    return ;
                }
                check_list.insert(id);
            }
            process[id]++;
            if(process[id]==this->stage_num){
                using return_type = typename std::result_of<F(Args...)>::type;
                std::future<return_type> ret = ThreadPools[process[id]-1]->enqueue(func,args...);
                result[id]= ret.share();
                cv.notify_all();
            }
            else if(process[id]<this->stage_num){
                ThreadPools[process[id]-1]->enqueue(func,args...);
            }
        }
        
    }
    template<class T>
    auto get_result(size_t id){
        {
            std::unique_lock<std::mutex> lock(this->lock);
            cv.wait(lock, [&]{ return process[id] == this->stage_num;});
            cv.notify_all();
        }
        return std::any_cast<std::shared_future<T>>(result[id]);
    }
    ~PipeLine(){
        {
            std::unique_lock<std::mutex> lock(this->lock);
            finish = true;
        }
        for(auto &id:check_list){
            {
                std::unique_lock<std::mutex> lock(this->lock);
                cv.wait(lock, [&]{ return process[id] == this->stage_num; });
                cv.notify_all();
            }
        }
        std::cout<<"pipeline end\n";
    }
};
#endif
