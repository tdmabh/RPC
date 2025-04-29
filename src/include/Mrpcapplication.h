#include <mutex>
#include "Mrpcconfig.h"
#include "Mrpccontroller.h"
#include "Mrpcchannel.h"
// 基础类，负责框架初始化
// 单例类，RPC框架基础入口,只读取一次配置文件
class MrpcApplication
{
public:
    static void Init(int argc, char **argv);
    static MrpcApplication &GetInstance();
    static void deleteInstance();
    static Mrpcconfig& GetConfig();

private:
    static Mrpcconfig m_config;
    static MrpcApplication * m_application;
    static std::mutex m_mutex;
    //放到private这，防止外部new构造，只能用GetInstance
    MrpcApplication(){};
    ~MrpcApplication(){};
    MrpcApplication(const MrpcApplication&) = delete;
    MrpcApplication(MrpcApplication&&) = delete;
};