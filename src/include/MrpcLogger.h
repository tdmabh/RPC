#pragma once
#include<glog/logging.h>
#include<string>

class MrpcLogger
{
public:
    explicit MrpcLogger(const char *argv0){
        google::InitGoogleLogging(argv0);
        FLAGS_colorlogtostderr = true;//彩色日志
        FLAGS_logtostderr = true;//标准错误
    }
    ~MrpcLogger(){
        google::ShutdownGoogleLogging();
    }
    static void Info(const std::string &message){
        LOG(INFO)<<message;
    }
    static void Warning(const std::string &message){
        LOG(WARNING)<<message;
    }
    static void ERROR(const std::string &message){
        LOG(ERROR)<<message;
    }
    
    static void Fatal(const std::string &message){
        LOG(FATAL)<<message;
    }
private:
    //防止错误复制
    MrpcLogger(const MrpcLogger&) = delete;
    MrpcLogger& operator=(const MrpcLogger&) = delete;
};