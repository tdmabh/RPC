#include "Mrpcprovider.h"
#include "Mrpcapplication.h"
#include "Mrpcheader.pb.h"
#include "MrpcLogger.h"
#include <iostream>

// 注册服务的service和method
void MrpcProvider::NotifyService(google::protobuf::Service *service)
{
    // 客户端想要调用的service和method就存储在ServiceInfo结构中
    ServiceInfo service_info;

    // 所有由protobuf生成的服务类都继承自google::protobuf::Service，通过创建子类对象实现动态多态
    // 通过动态多态调用 service->GetDescriptor()，GetDescriptor() 方法会返回 protobuf 生成的服务类的描述信息（ServiceDescriptor）
    // 这里的service会通过proto文件定义的接口进行实现并在server.cc中传入
    const google::protobuf::ServiceDescriptor *psd = service->GetDescriptor();

    //获取服务类后，提取服务和方法列表，进行注册
    std::string service_name = psd->name();
    int method_count = psd->method_count();

    std::cout<<"service name = " <<service_name<<std::endl;

    for(int i = 0;i<method_count;i++){
        //获取方法描述
        const google::protobuf::MethodDescriptor *pmd = psd->method(i);
        std::string method_name = pmd->name();
        std::cout<<"method name = "<< method_name<<std::endl;
        //在service的方法列表中存入 方法名和方法描述符
        service_info.method_map.emplace(method_name,pmd);
    }
    service_info.service = service;
    service_map.emplace(service_name,service_info);
}

//RPC节点启动
void MrpcProvider::Run(){
    //读取配置文件中的ip和port
    std::string ip =  MrpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    //用atoi把char*字符串类型转换成int类型
    int port = atoi(MrpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    
    //调用muduo库创建网络对象
    muduo::net::InetAddress address(ip,port);
    
    //创建TcpServer对象,"Mrpcprovider"是用来调式和日志的
    std::shared_ptr<muduo::net::TcpServer> server = std::make_shared<muduo::net::TcpServer>(&event_loop,address,"MrpcProvider");

    //分别绑定连接回调函数和消息回调函数，分离连接和消息业务
    server->setConnectionCallback(std::bind(&MrpcProvider::OnConnection,this,std::placeholders::_1));
    //&MrpcProvider::OnMessage取成员函数指针，this用于绑定对象，哪个对象调用这个类方法，就绑定哪个
    //bind()把成员函数 + 对象 + 参数占位符一起“绑定”成一个函数对象，便于异步调用	
    server->setMessageCallback(std::bind(&MrpcProvider::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

    //设置muduo线程数
    server->setThreadNum(4);

    //把服务端rpc节点上的服务都注册到zookeeper上
    ZkClient zkclient;
    zkclient.Start();
    for(auto &sp:service_map){
        // service_name 在ZooKeeper中的目录是"/"+service_name
        //这里的service_path和method_path指的是在zookeeper中的path
        std::string service_path = "/"+ sp.first;
        zkclient.Create(service_path.c_str(),nullptr,0);
        for(auto &mp : sp.second.method_map){
            std::string method_path = service_path+"/"+mp.first;
            //method_path_data中存ip：port，用来找到服务地址
            char method_path_data[128] = {0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);
            //这里ZOO_EPHEMERAL标识临时节点
            zkclient.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }
    std::cout << "RpcProvider start service at ip:"<< ip <<"port:"<< std::endl;

    //启动网络服务
    server->start();
    event_loop.loop();
}

void MrpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn){
    if(!conn->connected()){
        //如果连接已经关闭，就断开连接
        conn->shutdown();
    }

}

void MrpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,muduo::net::Buffer *buffer,muduo::Timestamp receive_time){
    std::cout<<"OnMessage"<<std::endl;

    //在网络缓冲区中读取RPC调用的字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    //讲二进制数据转换成protobuf对象
    google::protobuf::io::ArrayInputStream raw_input(recv_buf.data(),recv_buf.size());
    google::protobuf::io::CodedInputStream coded_input(&raw_input);

    uint32_t header_size{};
    coded_input.ReadVarint32(&header_size);

    //从header_size中读取rpc请求详细信息
    std::string rpc_header_str;
    Mrpc::RpcHeader MrpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size{};

    //先把rpc消息头读出来,设置读取限制并读取
    google::protobuf::io::CodedInputStream::Limit msg_limit = coded_input.PushLimit(header_size);
    coded_input.ReadString(&rpc_header_str,header_size);
    //恢复限制
    coded_input.PopLimit(msg_limit);

    //读取消息头后面的信息(方法的参数信息)
    if(MrpcHeader.ParseFromString(rpc_header_str)){
        service_name = MrpcHeader.service_name();
        method_name = MrpcHeader.method_name();
        args_size = MrpcHeader.args_size();
    }else{
        MrpcLogger::ERROR("Mrpcheader parse error");
        return;
    }

    std::string args_str;
    bool read_args_success = coded_input.ReadString(&args_str,args_size);
    if(!read_args_success){
        MrpcLogger::ERROR("read args error");
        return;
    }

    //在服务端找到服务和方法对象
    auto it = service_map.find(service_name);
    if(it == service_map.end()){
        std::cout<<service_name<<"does not exist !"<<std::endl;
        return;
    }
    auto mit = it->second.method_map.find(method_name);
    if(mit == it->second.method_map.end()){
        std::cout<<method_name<<"does not exist !"<<std::endl;
        return;
    }

    google::protobuf::Service *service = it->second.service;//获取服务对象
    const google::protobuf::MethodDescriptor *method = mit->second;//获取方法对象

    //生成rpc调用的请求的request和响应的response
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();//动态创建请求对象
    //如果反序列化成功，会在request中存入参数信息，如果没有成功就打印错误
    if(!request->ParseFromString(args_str)){
        std::cout<<service_name<<"."<<method_name<<"parse error!"<<std::endl;
        return;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();
    //绑定回调函数，方法调用,protobuf框架会自动调用SendRpcResponse，完成响应
    google::protobuf::Closure *done = google::protobuf::NewCallback<MrpcProvider,
                                                                    const muduo::net::TcpConnectionPtr &,
                                                                    google::protobuf::Message*>(this,
                                                                                                &MrpcProvider::SendRpcResponse,
                                                                                                conn,response);
    //根据客户端的rpc请求，调用rpc节点上发布的方法
    service->CallMethod(method,nullptr,request,response,done);
    
}

void MrpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn,google::protobuf::Message* response){
    std::string response_str;
    //进行序列化，如果序列化成功，就通过网络把rpc方法执行结果返回给请求方
    if(response->SerializeToString(&response_str)){
        conn->send(response_str);
    }else{
        std::cout<<"Serialize error!"<<std::endl;
    }
}

MrpcProvider::~MrpcProvider(){
    std::cout<<"~MrpcProvider()"<<std::endl;
    event_loop.quit();
}
