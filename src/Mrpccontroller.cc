#include "Mrpccontroller.h"

Mrpccontroller::Mrpccontroller(){
    m_failed = false;
    m_errText = "";
}

void Mrpccontroller::Reset(){
    m_failed = false;
    m_errText = "";
}

bool Mrpccontroller::Failed() const{
    return m_failed;
}

std::string Mrpccontroller::ErrorText() const{
    return m_errText;
}

void Mrpccontroller::SetFailed(const std::string &reason){
    m_failed = true;
    m_errText = reason;
}

// 以下功能未实现，是RPC服务端提供的取消功能
// 开始取消RPC调用（未实现）
void Mrpccontroller::StartCancel() {
    // 目前为空，未实现具体功能
}

// 判断RPC调用是否被取消（未实现）
bool Mrpccontroller::IsCanceled() const {
    return false;  // 默认返回false，表示未被取消
}

// 注册取消回调函数（未实现）
void Mrpccontroller::NotifyOnCancel(google::protobuf::Closure* callback) {
    // 目前为空，未实现具体功能
}
