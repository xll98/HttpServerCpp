#pragma once
#include <string>
#include <string.h>
#include <map>
#include <set>
#include <functional>
#include "mongoose.h"

typedef enum
{
	Content_json			= 0,		// 以json格式返回
	Content_file			= 1,		// 以二进制文件格式返回
	Content_jpg				= 2,		// 以jpg图片格式返回
	Content_html			= 3,		// 以html格式返回
	Content_text			= 4,		// 以text格式返回
	Content_404				= 5			// 以404方式返回
}Content_type;

typedef void OnRspCallback(mg_connection *c,const char* ptr, int datalen, Content_type content_type);
// 定义http请求handler
using ReqHandler = std::function<bool (std::string, const char* body_data,int body_len, mg_connection *c, OnRspCallback)>;

class HttpServer
{
public:
	HttpServer() {}
	~HttpServer() {}
	void Init(const std::string &port); // 初始化设置
	void SetIndexHtml(const std::string &html);		// 设置首页html
	bool Start(); // 启动httpserver
	bool Close(); // 关闭
	void AddHandler(const std::string &url, ReqHandler req_handler); // 注册事件处理函数
	void RemoveHandler(const std::string &url); // 移除事件处理函数
	static std::map<std::string, ReqHandler> s_handler_map; // 回调函数映射表
	static std::string html_404;
	static std::string html_index;

private:
	// 静态事件响应函数
	static void OnHttpWebsocketEvent(mg_connection *connection, int event_type, void *event_data);

	static void HandleHttpEvent(mg_connection *connection, mg_http_message *http_req);
	static void SendHttpRsp(mg_connection *connection,const char* ptr,int datalen,Content_type content_type);
	std::string m_port;    // 端口
	mg_mgr m_mgr;          // 连接管理器
};