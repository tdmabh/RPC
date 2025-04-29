#include <iostream>
#include <string>
#include "../user.pb.h"
#include "Mrpcapplication.h"
#include "Mrpcprovider.h"

//通过rpc框架，把UserService提供的方法发布到远程调用

class UserService : public Muser::UserServiceRpc{
public:
    //登录方法的业务逻辑(本地方法)
    bool Login(std::string name,std::string pwd){
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name:" << name << "pwd:"<<pwd<<std::endl;
        return true;
    }

    //通过rpc发布的Login方法，重写protobuf的虚函数
    void Login(::google::protobuf::RpcController* controller,
                const ::Muser::LoginRequest* request,
                ::Muser::LoginResponse* response,
                ::google::protobuf::Closure* done){
        //从请求中获得用户名和密码
        std::string name = request->name();
        std::string pwd = request->pwd();

        //调用本地业务逻辑处理登录请求
        bool login_request = Login(name,pwd);

        //把本地结果存入response对象
        Muser::ResultCode * code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_request);
        
        //执行回调操作，框架会自动把响应发给客户端
        //这里在Mrpcprovider中，把done构造成了一个继承Closure的匿名子类，它里面绑定了Mrpcprovider的this指针
        //这里的Run不是MrpcProvider里面的Run，而是这个Closure的一个虚函数，它执行的是传入的函数指针，在我的实现中，是 &MrpcProvider::SendRpcResponse
        done->Run();

    }

};

int main(int argc,char ** argv){
    //调用框架初始化，解析命令行参数并加载配置文件
    MrpcApplication::Init(argc,argv);

    //创建一个RPC服务提供者对象
    MrpcProvider provider;
    
    //把UserService对象(上面定义的实现Login本地方法的一个类)
    provider.NotifyService(new UserService());

    //启动RPC服务节点，进入阻塞状态，等待RPC请求
    provider.Run();

}