#include "Mrpcapplication.h"
#include <cstdlib>
#include <unistd.h>

Mrpcconfig MrpcApplication::m_config;
std::mutex MrpcApplication::m_mutex;
MrpcApplication *MrpcApplication::m_application = nullptr;

// 初始化函数，解析命令行参数和加载配置文件
void MrpcApplication::Init(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "格式：command -i <配置文件路径>" << std::endl;
        exit(EXIT_FAILURE);
    }

    int o;
    std::string config_file;
    // getopt命令解析命令行参数
    while (-1 != (o = getopt(argc, argv, "i:")))
    {
        switch (o)
        {
        case 'i': // 验证出现参数是否i
            config_file = optarg;
            break;
        case '?':// 如果出现未知参数，提示正确格式
            std::cout << "格式: command -i <配置文件路径>" << std::endl;
            exit(EXIT_FAILURE);
            break;
        case ':': // 如果-i后面没有跟参数，提示正确格式并退出
            std::cout << "格式: command -i <配置文件路径>" << std::endl;
            exit(EXIT_FAILURE);
            break;
        default:
            break;
        }
    }
    //config_file作为路径（实际上是指针）能取出配置文件的内容，进而进行解析
    m_config.LoadConfigFile(config_file.c_str());
}

MrpcApplication &MrpcApplication::GetInstance(){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_application == nullptr){
        m_application = new MrpcApplication();
        atexit(deleteInstance);//atexit()函数，注册完之后，程序退出前会自动执行（main函数结束或者exit()时）
    }
    return *m_application;
}

void MrpcApplication::deleteInstance(){
    if(m_application){
        delete m_application;
    }
}

Mrpcconfig& MrpcApplication::GetConfig(){
    return m_config;
}



