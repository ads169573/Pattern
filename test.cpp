#include "ThreadPool.hpp"
#include "PipeLine.hpp"
#include <omp.h>

#define openMP 0
#define THREAD_POOL 0
#define PIPELINE 1
long long func(long long num){
    
    long long sum=0;
    for(long long i=0;i<num;++i) sum+=1;
    return sum;
}
void st3(std::shared_ptr<PipeLine> &handle,size_t id){
    int a = 1e9;
    while(a--){}
    std::cout<<"st3\n";
}
void st2(std::shared_ptr<PipeLine> &handle,size_t id){
    int a = 1e9;
    while(a--){}
    std::cout<<"st2\n";
    handle->run(id,st3,std::ref(handle),id);
}
void st1(std::shared_ptr<PipeLine> &handle,size_t id){
    int a = 1e9;
    while(a--){}
    std::cout<<"st1\n";
    handle->run(id,st2,std::ref(handle),id);
}

int main()
{
#if PIPELINE
    std::vector<size_t> config(3,1);
    auto pipeline = std::make_shared<PipeLine>(3,config);
    pipeline->run(0,st1,std::ref(pipeline),0);
    pipeline->run(1,st1,std::ref(pipeline),1);
    pipeline->run(2,st1,std::ref(pipeline),2);
    auto ret = (pipeline->get_result<void>(0));
    ret.wait();
    ret = (pipeline->get_result<void>(1));
    ret.wait();
    ret = (pipeline->get_result<void>(2));
    ret.wait();
#endif
#if THREAD_POOL
    ThreadPool threadpool(4);
    long long ans=0;
    std::mutex ans_lock;
#if openMP
#pragma omp parallel num_threads(4)
    {
        auto ret = func(2500000000);
        {
            std::unique_lock<std::mutex> lock(ans_lock);
            ans += ret;
        }
    }
#else
    std::vector<std::future<long long>> v(4);
    for(long long i=0;i<4;++i){
        v[i] = threadpool.enqueue(func,2500000000);
    }
    for(int i=0;i<4;++i) ans+=v[i].get();
#endif
    std::cout<<ans<<std::endl;
#endif
    return 0;
}