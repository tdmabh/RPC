#include "Mrpcapplication.h"
#include "../user.pb.h"
#include "Mrpccontroller.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include "MrpcLogger.h"

//发送RPC请求，模拟客户端调用远程服务的请求
void send_request(int thread_id,std::atomic<int> &success_count,std::atomic<int> &fail_count){
    //用一个stub对象，用于调用远程RPC方法
    Muser::UserServiceRpc_Stub stub(new MrpcChannel(false));

    //设置RPC方法请求参数
    Muser::LoginRequest request;
    request.set_name("User_1");
    request.set_pwd("123456");

    //定义RPC方法响应参数
    Muser::LoginResponse response;
    Mrpccontroller controller; //创建控制器对象，处理调用过程中的问题

    //调用Login方法
    stub.Login(&controller,&request,&response,nullptr);

    //检查RPC调用是否成功
    if(controller.Failed()){
        std::cout <<controller.ErrorText() <<std::endl;
        fail_count++;
    }else{
        if(0 == response.result().errcode()){//如果调用成功了，就进一步检查调用过程中有没有错误码（业务逻辑的错误码）
            std::cout << "rpc Login response success: " <<response.success()<<std::endl;
            success_count++;//用于日志测试
        }else {
            std::cout << "rpc login response error: "<<response.result().errmsg() <<std::endl;
            fail_count++;
        }
    }
}

int main(int argc,char **argv){
    //初始化RPC框架，解析命令行参数并加载配置文件
    MrpcApplication::Init(argc,argv);

    //创建日志对象
    MrpcLogger logger("MyRPC");

    const int thread_count = 10;      //并发线程数
    const int requests_per_thread = 1; //每个线程发送的请求数

    std::vector<std::thread> threads;  //存储线程对象的容器
    std::atomic<int> success_count(0); //原子操作类型，成功请求的计数器
    std::atomic<int> fail_count(0);    

    auto start_time = std::chrono::high_resolution_clock::now();

    //启动多线程进行并发测试
    for(int i = 0;i < thread_count;i++){
        threads.emplace_back([argc,argv,i,&success_count,&fail_count,requests_per_thread](){for(int j = 0;j < requests_per_thread;j++){send_request(i,success_count,fail_count);}});
    }//传入lambda表达式，表示线程会执行lambda表达式的内容，并把这个线程存入线程对象
    
    for(auto &t: threads){
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time-start_time;

    //输出日志
    LOG(INFO) << "Total requests: " << thread_count *requests_per_thread;
    LOG(INFO) << "Success count: " << success_count;
    LOG(INFO) << "Fail count: " << fail_count;
    LOG(INFO) << "Elapsed time: " << elapsed.count() << " seconds";
    LOG(INFO) << "QPS: " << (thread_count * requests_per_thread)/elapsed.count();

    return 0;


}








 