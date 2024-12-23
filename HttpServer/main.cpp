
#include "http_server.h"
#define WEB_PORT "8080"

bool handle_reply_file(std::string url, const char* body_data, int body_len, mg_connection *c, OnRspCallback rsp_callback);
bool handle_reply_string(std::string url, const char* body_data, int body_len, mg_connection *c, OnRspCallback rsp_callback);
bool handle_reply_jpg(std::string url, const char* body_data, int body_len, mg_connection *c, OnRspCallback rsp_callback);

int main(){
    HttpServer* http_server = new HttpServer();
	http_server->Init(WEB_PORT);
    /* 系统接口：*/
    http_server->AddHandler("/api/getfile", handle_reply_file);                   // 获取文件内容
    http_server->AddHandler("/api/string", handle_reply_string);                   // 获取文件内容
    http_server->AddHandler("/api/pic", handle_reply_jpg);                   // 获取文件内容
    http_server->Start();
    return 0;
}

bool handle_reply_file(std::string url, const char* body_data, int body_len, mg_connection *c, OnRspCallback rsp_callback){

    const char *file_path = "testfile";
    FILE *file = fopen(file_path, "rb");  // 打开文件（以二进制模式）

    if (file == NULL) {
        // 文件打开失败，返回 404 Not Found
        rsp_callback(c, "打开文件失败", 0, Content_404);
        return false;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);      // 定位到文件末尾
    long file_size = ftell(file);  // 获取当前位置（即文件大小）
    fseek(file, 0, SEEK_SET);      // 重置文件指针到文件开头

    // 分配缓冲区来存放文件内容
    char *file_buffer = (char *)malloc(file_size);
    if (file_buffer == NULL) {
        // 内存分配失败，返回 500 Internal Server Error
        rsp_callback(c, "malloc 失败", 0, Content_404);
        fclose(file);
        return false;
    }

    // 读取文件内容
    fread(file_buffer, 1, file_size, file);
    fclose(file);
    printf("file_size: %ld\n", file_size);
    rsp_callback(c, file_buffer, file_size, Content_file);
    // 释放分配的内存
    free(file_buffer);
    return true;
}

bool handle_reply_jpg(std::string url, const char* body_data, int body_len, mg_connection *c, OnRspCallback rsp_callback){

    const char *file_path = "test.jpg";
    FILE *file = fopen(file_path, "rb");  // 打开文件（以二进制模式）

    if (file == NULL) {
        // 文件打开失败，返回 404 Not Found
        rsp_callback(c, "打开文件失败", 0, Content_404);
        return false;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);      // 定位到文件末尾
    long file_size = ftell(file);  // 获取当前位置（即文件大小）
    fseek(file, 0, SEEK_SET);      // 重置文件指针到文件开头

    // 分配缓冲区来存放文件内容
    char *file_buffer = (char *)malloc(file_size);
    if (file_buffer == NULL) {
        // 内存分配失败，返回 500 Internal Server Error
        rsp_callback(c, "malloc 失败", 0, Content_404);
        fclose(file);
        return false;
    }

    // 读取文件内容
    fread(file_buffer, 1, file_size, file);
    fclose(file);
    printf("file_size: %ld\n", file_size);
    rsp_callback(c, file_buffer, file_size, Content_jpg);
    // 释放分配的内存
    free(file_buffer);
    return true;
}

bool handle_reply_string(std::string url, const char* body_data, int body_len, mg_connection *c, OnRspCallback rsp_callback){
    rsp_callback(c, "http server demo!", 17, Content_text);
    return true;
}
