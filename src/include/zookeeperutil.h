#pragma once 
#include<semaphore.h>
#include<zookeeper/zookeeper.h>
#include<string>

class ZkClient
{
public:
    ZkClient();
    ~ZkClient();
    //启动zkserver
    void Start();
    //在zkserver中，根据指定的path创建服务节点
    void Create(const char *path,const char* data,int datalen,int state = 0);
    //根据参数读取指定的znode节点，读数据用
    std::string GetData(const char* path);
private:
    //zk客户端句柄
    zhandle_t* m_zhandle;
};