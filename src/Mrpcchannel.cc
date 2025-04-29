#include "Mrpcchannel.h"
#include "Mrpcheader.pb.h"
#include "zookeeperutil.h"
#include "Mrpcapplication.h"
#include "Mrpccontroller.h"
#include "memory"
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "MrpcLogger.h"

std::mutex g_data_mutex;

//RPC调用的核心方法，负责将客户端的请求序列化并发送到服务端，同时接收服务端的响应
void MrpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method, 
                            google::protobuf::RpcController *controller, 
                            const google::protobuf::Message *request, 
                            google::protobuf::Message *response, 
                            google::protobuf::Closure *done){
    if(-1 == m_clientfd){
        //如果客户端还没初始化，就去获取服务和方法的名字
        const google::protobuf::ServiceDescriptor *sd = method->service();
        service_name = sd->name();
        method_name = method->name();

        //运行客户端zookeeper，通过ip和port找到zookeeper服务器
        ZkClient zkCli;
        zkCli.Start();
        std::string host_data = QueryServiceHost(&zkCli,service_name,method_name,m_idx);
        m_ip = host_data.substr(0,m_idx);
        std::cout<<"ip: "<<m_ip<<std::endl;
        m_port = atoi(host_data.substr(m_idx+1,host_data.size()-m_idx).c_str());
        std::cout<<"port: "<<m_port<<std::endl;

        //连接服务器
        auto rt = newConnect(m_ip.c_str(),m_port);
        if(!rt){
            LOG(ERROR) <<"connect server error"<<std::endl;
            return;
        }
        else{
            LOG(INFO)<<"connect server success"<<std::endl;
        }

    }

    //把请求参数序列化为字符串，并计算长度
    uint32_t args_size{};
    std::string args_str;
    if(request->SerializePartialToString(&args_str)){
        args_size = args_str.size();
    }else{
        controller->SetFailed("serialize request fail");
        return;
    }

    //定义rpc请求头消息
    Mrpc::RpcHeader Mrpcheader;
    Mrpcheader.set_service_name(service_name);
    Mrpcheader.set_method_name(method_name);
    Mrpcheader.set_args_size(args_size);

    //把rpc头部信息序列化为字符串，并计算长度
    uint32_t header_size=0;
    std::string rpc_header_str;
    if(Mrpcheader.SerializePartialToString(&rpc_header_str)){
        header_size = rpc_header_str.size();
    }else{
        controller->SetFailed("serialize rpc header fail");
        return;
    }

    //把头部信息和头部报文拼接
    std::string send_rpc_str;
    {
        google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);
        coded_output.WriteVarint32(static_cast<uint32_t>(header_size));
        coded_output.WriteString(rpc_header_str);
    }
    send_rpc_str += args_str;

    //发送rpc请求到服务器
    if(-1 == send(m_clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0)){
        close(m_clientfd);//发送失败，关闭socket
        char errtxt[512] = {};
        std::cout <<"send error:"<<strerror_r(errno,errtxt,sizeof(errtxt))<<std::endl;
        controller->SetFailed(errtxt);
        return;
    }

    //接收服务器响应
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if(-1 == (recv_size = recv(m_clientfd,recv_buf,1024,0))){
        char errtxt[512] = {};
        std::cout<<"recv error"<<strerror_r(errno,errtxt,sizeof(errtxt))<<std::endl;
        controller->SetFailed(errtxt);
        return;
    }

    //接收到服务器数据后，把数据序列反序列化为response对象
    if(!response->ParseFromArray(recv_buf,recv_size)){
        close(m_clientfd);
        char errtxt[512] = {};
        std::cout << "parse error"<< strerror_r(errno,errtxt,sizeof(errtxt))<<std::endl;
        controller->SetFailed(errtxt);
        return;
    }

    close(m_clientfd);
}

//创建新的socket连接的方法
bool MrpcChannel::newConnect(const char *ip,uint16_t port){
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == clientfd){
        char errtxt[512] = {0};
        std::cout<<"socket error"<<strerror_r(errno,errtxt,sizeof(errtxt))<<std::endl;
        LOG(ERROR)<<"socket error:"<< errtxt;
        return false;
    }

    //设置服务器地址信息
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    //连接服务器
    if(-1 == connect(clientfd,(struct sockaddr *)&server_addr,sizeof(server_addr))){
        close(clientfd);
        char errtxt[512] = {0};
        std::cout<<"connect error"<<strerror_r(errno,errtxt,sizeof(errtxt))<<std::endl;
        LOG(ERROR) << "connect server error" <<errtxt;
        return false;
    }
    m_clientfd = clientfd;
    return true;
} 

//从zookeeper查询服务地址
std::string MrpcChannel::QueryServiceHost(ZkClient *zkclient,std::string serice_name,std::string method_name,int &idx){
    std::string method_path = "/" + service_name + "/" + method_name;
    std::cout << "method_path:" << method_path <<std::endl;

    std::unique_lock<std::mutex> lock(g_data_mutex);
    std::string host_data_1 = zkclient->GetData(method_path.c_str());
    lock.unlock();

    if(host_data_1 == ""){
        LOG(ERROR) <<method_path + "dose not exit !";
        return " ";
    }

    idx = host_data_1.find(":");
    if(idx == -1){
        LOG(ERROR) << method_path + "address is invalid!";
        return " ";
    }
    
    return host_data_1;
    
}

//构造函数，延迟连接
MrpcChannel::MrpcChannel(bool connectNow) : m_clientfd(-1),m_idx(0){
    if(!connectNow){
        return;
    }

    //尝试连接服务器
    auto rt= newConnect(m_ip.c_str(),m_port);
    int count = 3;
    while(!rt && count--){
        rt = newConnect(m_ip.c_str(),m_port);
    }
}


