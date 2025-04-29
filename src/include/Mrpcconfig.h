#pragma once
#include <unordered_map>
#include <string>

//用于解析配置文件
class Mrpcconfig{
public:
    //加载配置文件
    void LoadConfigFile(const char*config_file);
    //根据key查找value
    std::string Load(const std::string &key);
private:
    std::unordered_map<std::string,std::string> config_map;
    //去掉字符串前后空格
    void Trim(std::string &read_buf);
};