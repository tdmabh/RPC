#pragma once
#include "google/protobuf/service.h"
#include "zookeeperutil.h"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <google/protobuf/descriptor.h>
#include <functional>
#include <string>
#include <unordered_map>

// 核心功能
class MrpcProvider
{
public:
    // publish method to the outside
    void NotifyService(google::protobuf::Service *service);
    ~MrpcProvider();
    // start the rpc
    void Run();

private:
    muduo::net::EventLoop event_loop;
    struct ServiceInfo
    {
        google::protobuf::Service *service;
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> method_map;
    };
    // save the objects of services and methods
    std::unordered_map<std::string, ServiceInfo> service_map;

    void OnConnection(const muduo::net::TcpConnectionPtr &conn);                                                           // 连接回调
    void OnMessage(const ::muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp receive_time); // 消息回调
    void SendRpcResponse(const ::muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response);                 // 处理完之后发送响应给客户端
};