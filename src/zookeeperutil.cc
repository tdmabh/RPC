#include "zookeeperutil.h"
#include <mutex>
#include <condition_variable>
#include "MrpcLogger.h"
#include "Mrpcapplication.h"

//封装zookeeper的功能，实现一个ZkClient类
std::mutex cv_mutex;
std::condition_variable cv; //cv条件变量，实现线程间通信
bool is_connected = false; // 验证Zookeeper客户端是否连接上

//全局watcher，监控zookeeper服务器通知
void global_watcher(zhandle_t* zh,int type,int status,const char* path,void *watcherCtx){
    if(type == ZOO_SESSION_EVENT){//如果回调消息类型是：和会话相关的事件
        if(status == ZOO_CONNECTED_STATE){//zookeeper客户端和服务器连接成功
            std::lock_guard<std::mutex> lock(cv_mutex);
            is_connected = true;
        }
    }
    cv.notify_all();//通知所有线程
}

//构造函数，初始化zookeeper的客户端句柄为空
ZkClient::ZkClient():m_zhandle(nullptr){}

ZkClient::~ZkClient(){
    if(m_zhandle!= nullptr){
        zookeeper_close(m_zhandle);
    }
}

//启动客户端zookeeper，连接zookeeper服务器
void ZkClient::Start(){
    //从配置文件中读取ip和port
    //通过GetInstance返回的&MrpcApplication类型继续调用GetCnfig()，然后再通过返回的Mrpcconfig类型调用Load
    std::string host = MrpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MrpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host+":"+port;
    /*
    zookeeper_mt：多线程版本
    ZooKeeper的API客户端程序提供了三个线程:
    1. API调用线程
    2. 网络I/O线程（使用pthread_create和poll）
    3. watcher回调线程（使用pthread_create）
    调用zookeeper_init之后这些会发生
    */
    //zookeeper初始化zookeeper客户端
    m_zhandle = zookeeper_init(connstr.c_str(),global_watcher,6000,nullptr,nullptr,0);
    if(m_zhandle == nullptr){
        LOG(ERROR) << "zookeeper_init_error";
        exit(EXIT_FAILURE);
    }

    std::unique_lock<std::mutex> lock(cv_mutex);//一个互斥锁管理cv_mutex，加锁
    cv.wait(lock,[]{return is_connected;});//wait会释放锁并阻塞，等待其他线程notify之后，会加锁再判断is_connected，满足就继续，不满足继续阻塞
    LOG(INFO) << "zookeeper_init_success";
}

    void ZkClient::Create(const char* path,const char* data,int datalen,int state){
        char path_buffer[128];
        int bufferlen = sizeof(path_buffer);

        //检查节点是否存在
        int flag = zoo_exists(m_zhandle,path,0,nullptr);
        if(flag == ZNONODE){
            flag = zoo_create(m_zhandle,path,data,datalen,&ZOO_OPEN_ACL_UNSAFE,state,path_buffer,bufferlen);
            if(flag == ZOK){
                LOG(INFO) <<"znode create success... path:"<<path;
            }else{
                LOG(ERROR) <<"znode create failed... path:"<<path;
                exit(EXIT_FAILURE);
            }
        }
    }

    //获取节点数据
    std::string ZkClient::GetData(const char *path){
        char buf[64];
        int bufferlen = sizeof(buf);

        int flag = zoo_get(m_zhandle,path,0,buf,&bufferlen,nullptr);
        if(flag != ZOK ){
            LOG(ERROR) << "zoo_get error";
            return "";
        }
        else{
            return buf;
        }
        return "";
    }