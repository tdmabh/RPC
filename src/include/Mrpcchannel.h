#pragma once 
#include <google/protobuf/service.h>
#include "zookeeperutil.h"

//给客户端服务
//继承自google::protobuf::RpcChannel,目的是为了给客户端进行方法调用的时候，统一接收
class MrpcChannel : public google::protobuf::RpcChannel{
public:
    MrpcChannel(bool connectedNow);
    virtual ~MrpcChannel(){};
    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done) override;
private:
    int m_clientfd;
    std::string service_name;
    std::string m_ip;
    uint16_t m_port;
    std:: string method_name;
    int m_idx;//划分ip和port的下标
    bool newConnect(const char *ip,uint16_t port);
    std::string QueryServiceHost(ZkClient *zkclient,std::string service_name,std::string method_name,int &idx);
};