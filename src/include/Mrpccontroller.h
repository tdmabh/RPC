#pragma once
#include <google/protobuf/service.h>
#include <string>

//跟踪RPC调用的状态，并进行一定的控制
class Mrpccontroller : public google::protobuf::RpcController
{
public:
    Mrpccontroller();
    void Reset();
    bool Failed() const;
    std::string ErrorText() const;
    void SetFailed(const std::string &reason);
    
    
    void StartCancel();
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure* callback);

private:
    bool m_failed;//RPC方法执行过程中的状态
    std::string m_errText;//RPC方法执行过程中的错误信息
};