# RPC项目

## 项目简介
本项目基于protobuf的c++分布式网络通信框架，使用了zookeeper作为服务中间件，负责解决在分布式服务部署中 服务的发布与调用、消息的序列与反序列化、网络包的收发等问题，使其能提供高并发的远程函数调用服务，可以让使用者专注于业务，快速实现微服务的分布式部署。

## 运行环境

Ubuntu 22.04 LTS

### 环境依赖
- muduo网络库
- zookeeper
- Protobuf
- boost
- Glog日志库

## 编译指令

第一步：进入到Mrpc文件
```shell
cd Mrpc
```

第二步：生成项目可执行程序
```shell
mkdir build && cd build && cmake .. && make -j${nproc} 
```

第三步：然后进入到example文件夹下，找到user.proto文件执行以下命令,会生成user.pb.h和user.pb.cc：
```shell
protoc --cpp_out=. user.proto
```

第四步：进入到src文件下，找到Mrpcheader.proto文件同样会生成如上pb.h和pb.cc文件
```shell
protoc --cpp_out=. Mrpcheader.proto
```

第五步：进入到bin文件夹下,分别运行./server和./client，即可完成服务发布和调用。
server:
```shell
./server -i ./test.conf
```

client:
```shell
./client -i ./test.conf
```

**注意**： 需要重新编译只需要在build目录下执行MAKE -J${4} 即可。


## 整体的框架

- **muduo库**：负责数据流的网络通信，采用了多线程epoll模式的IO多路复用，让服务发布端接受服务调用端的连接请求，并由绑定的回调函数处理调用端的函数调用请求。

- **Protobuf**：负责RPC方法的注册，数据的序列化和反序列化，相比于文本存储的XML和JSON来说，Protobuf是二进制存储，且不需要存储额外的信息，效率更高。

- **Zookeeper**：负责分布式环境的服务注册，记录服务所在的IP地址以及端口号，可动态地为调用端提供目标服务所在发布端的IP地址与端口号，方便服务所在IP地址变动的及时更新。

- **TCP沾包问题处理**：定义服务发布端和调用端之间的消息传输格式，记录方法名和参数长度，防止粘包。

- **Glog日志库**：后续增加了Glog的日志库，进行异步的日志记录。

