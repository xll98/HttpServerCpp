#include <utility>
#include "http_server.h"

std::map<std::string, ReqHandler> HttpServer::s_handler_map;

static const char *mg_http_status_code_str(int status_code) {
  switch (status_code) {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 102: return "Processing";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 207: return "Multi-Status";
    case 208: return "Already Reported";
    case 226: return "IM Used";
    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 307: return "Temporary Redirect";
    case 308: return "Permanent Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Payload Too Large";
    case 414: return "Request-URI Too Long";
    case 415: return "Unsupported Media Type";
    case 416: return "Requested Range Not Satisfiable";
    case 417: return "Expectation Failed";
    case 418: return "I'm a teapot";
    case 421: return "Misdirected Request";
    case 422: return "Unprocessable Entity";
    case 423: return "Locked";
    case 424: return "Failed Dependency";
    case 426: return "Upgrade Required";
    case 428: return "Precondition Required";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 444: return "Connection Closed Without Response";
    case 451: return "Unavailable For Legal Reasons";
    case 499: return "Client Closed Request";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    case 506: return "Variant Also Negotiates";
    case 507: return "Insufficient Storage";
    case 508: return "Loop Detected";
    case 510: return "Not Extended";
    case 511: return "Network Authentication Required";
    case 599: return "Network Connect Timeout Error";
    default: return "";
  }
}

void HttpServer::Init(const std::string &port)
{
	m_port = port;
}

/**
 * @brief 初始化并启动HTTP服务器
 * 
 * @return 返回true表示成功启动，false表示启动失败
 **/
bool HttpServer::Start()
{
    // 初始化服务器管理器
    mg_mgr_init(&m_mgr);
    
    // 创建并绑定一个连接到指定端口，用于处理HTTP和WebSocket事件
    char url_buffer[64];
    memset(url_buffer, 0, sizeof(url_buffer));
    sprintf(url_buffer, "http://0.0.0.0:%s", m_port.c_str());
    mg_connection *connection = mg_http_listen(&m_mgr, url_buffer, HttpServer::OnHttpWebsocketEvent, NULL);  // Setup listener
    
    // 如果连接创建失败，则返回false
    if (connection == NULL)
        return false;
    
    // 输出服务器启动信息
    printf("starting http server at port: %s\n", m_port.c_str());
    
    // 主循环：持续轮询服务器管理器以处理事件
    while (true)
        mg_mgr_poll(&m_mgr, 500); // 500毫秒为一次轮询间隔
    
    // 如果服务器启动失败，返回true表示成功
    return true;
}

/**
 * 处理HTTP和WebSocket事件
 * 
 * 此函数用于区分并处理HTTP请求和WebSocket事件它作为事件回调函数，根据事件类型将事件数据转发给相应的处理函数
 * 
 * @param connection 一个表示与客户端连接的对象
 * @param event_type 事件类型，用于区分是HTTP请求还是WebSocket事件
 * @param event_data 事件数据，具体类型取决于event_type
 */
void HttpServer::OnHttpWebsocketEvent(mg_connection *connection, int event_type, void *event_data)
{
	//printf("http 收到 event_type: %d\n", event_type);
    // 区分http和websocket
    if (event_type == MG_EV_HTTP_MSG)
    {
        // 将事件数据转换为HTTP请求对象，并处理HTTP事件
        mg_http_message *http_req = (mg_http_message *)event_data;
        HandleHttpEvent(connection, http_req);
    }
}

/**
 * 检查HTTP请求的URI是否与指定的路由前缀匹配。
 * 
 * @param http_msg 指向HTTP消息的指针，其中包含请求的URI。
 * @param route_prefix 要比较的路由前缀字符串。
 * @return 如果URI与路由前缀匹配，则返回true；否则返回false。
 * 
 * 此函数使用mg_vcmp函数比较HTTP消息中的URI和给定的路由前缀。
 * 如果两者相等，函数返回true，表示路由匹配；否则返回false，表示路由不匹配。
 */
static bool route_check(mg_http_message *http_msg, const char *route_prefix)
{
    return mg_match(http_msg->uri, mg_str(route_prefix), NULL);
}

/**
 * @brief 向HttpServer添加请求处理程序
 * 该函数用于向服务器添加一个新的请求处理程序。如果给定URL已经存在处理程序，新的处理程序将不会被添加。
 * @param url  请求的URL路径
 * @param req_handler 请求处理程序的实例
 */
void HttpServer::AddHandler(const std::string &url, ReqHandler req_handler)
{
    // 检查给定的URL是否已经有对应的请求处理程序
    // 如果已存在，则不进行任何操作并退出函数
    if (s_handler_map.find(url) != s_handler_map.end())
        return;

    // 如果给定的URL没有对应的请求处理程序，则插入新的键值对
    // 这样，新的请求处理程序将被添加到服务器中，用于处理指定URL的请求
    s_handler_map.insert(std::make_pair(url, req_handler));
}

/**
 * @brief 从HTTP服务器中移除指定URL的请求处理程序
 * 
 * @param url  请求的URL路径
 * @param req_handler 请求处理程序的实例
 */
void HttpServer::RemoveHandler(const std::string &url)
{
    auto it = s_handler_map.find(url);
    if (it != s_handler_map.end())
        s_handler_map.erase(it);
}

/**
 * @brief 向客户端发送HTTP响应
 * 
 * 此函数负责构建并发送HTTP响应到指定的连接。
 * 
 * @param connection Mongoose连接对象指针，用于标识连接
 * @param ptr 指向要发送的数据的指针
 * @param datalen 要发送的数据长度（如果isStr为true，则此参数被忽略）
 * @param isStr 布尔值，指示ptr是否指向一个以null字符结尾的字符串
 */
void HttpServer::SendHttpRsp(mg_connection *connection,const char* ptr, int datalen, Content_type content_type)
{
	static const char *s_json_header =
	"HTTP/1.1 %d %s\r\n"
    "Content-Type: application/json\r\n"
    "Cache-Control: no-cache\r\n"
	"Content-Length: %d\r\n";

	static const char *s_stream_header =
	"HTTP/1.1 %d %s\r\n"
    "Content-Type: application/stream\r\n"
    "Cache-Control: no-cache\r\n"
	"Content-Length: %d\r\n";

	static const char *s_jpeg_header =
	"HTTP/1.1 %d %s\r\n"
    "Content-Type: image/jpeg\r\n"
    "Cache-Control: no-cache\r\n"
	"Content-Length: %d\r\n";

	static const char *s_html_header =
	"HTTP/1.1 %d %s\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Cache-Control: no-cache\r\n"
	"Content-Length: %d\r\n";

	static const char *s_text_header =
	"HTTP/1.1 %d %s\r\n"
    "Content-Type: text/plain; charset=UTF-8\r\n"
    "Cache-Control: no-cache\r\n"
	"Content-Length: %d\r\n";
	
	// 以json形式返回
	// 根据content_type的类型决定发送数据的方式
	switch (content_type){
		//json格式返回
		case Content_json:{
            //mg_http_reply(connection, 200, s_json_header, ptr);
			mg_printf(connection, s_json_header, 200, mg_http_status_code_str(200), datalen);
			mg_printf(connection, "%s", "\r\n");        // End HTTP headers
			mg_send(connection, ptr, datalen);
			connection->is_draining = 1;  // Tell mongoose to close this connection
			break;
		}
		//二进制格式返回
		case Content_file:{
            //mg_http_reply(connection, 200, s_stream_header, ptr);
			mg_printf(connection, s_stream_header, 200, mg_http_status_code_str(200), datalen);
			mg_printf(connection, "%s", "\r\n");        // End HTTP headers
			mg_send(connection, ptr, datalen);
			connection->is_draining = 1;  // Tell mongoose to close this connection
			break;
		}
		//图片jpg格式返回
		case Content_jpg:{
			mg_printf(connection, s_jpeg_header, 200, mg_http_status_code_str(200), datalen);
			mg_printf(connection, "%s", "\r\n");        // End HTTP headers
			mg_send(connection, ptr, datalen);
			connection->is_draining = 1;  // Tell mongoose to close this connection
			break;
		}
		//html格式返回
		case Content_html:{
            //mg_http_reply(connection, 200, s_html_header, ptr);
			mg_printf(connection, s_html_header, 200, mg_http_status_code_str(200), datalen);
			mg_printf(connection, "%s", "\r\n");        // End HTTP headers
			mg_send(connection, ptr, datalen);
			connection->is_draining = 1;  // Tell mongoose to close this connection
			break;
		}
		//以text格式返回
		case Content_text:{
            //mg_http_reply(connection, 200, s_text_header, ptr);
			mg_printf(connection, s_text_header, 200, mg_http_status_code_str(200), datalen);
			mg_printf(connection, "%s", "\r\n");        // End HTTP headers
			mg_send(connection, ptr, datalen);
			connection->is_draining = 1;  // Tell mongoose to close this connection
			break;
		}
		//404格式返回
		case Content_404:{
            //mg_http_reply(connection, 404, s_html_header, ptr);
			mg_printf(connection, s_html_header, 404, mg_http_status_code_str(404), datalen);
			mg_printf(connection, "%s", "\r\n");        // End HTTP headers
			mg_send(connection, ptr, datalen);
			connection->is_draining = 1;  // Tell mongoose to close this connection
			break;
		}
		default:{
			mg_http_reply(connection, 200, "", ptr);
            break;
		}
			
	}
}

bool HttpServer::Close()
{
	mg_mgr_free(&m_mgr);
	return true;
}

void HttpServer::SetIndexHtml(const std::string &html)
{
	html_index = html;
}

void HttpServer::HandleHttpEvent(mg_connection *connection, mg_http_message *http_req)
{
	std::string req_str = std::string(http_req->message.buf, http_req->message.len);
	std::string url = std::string(http_req->uri.buf, http_req->uri.len);
	auto it = s_handler_map.find(url);
	if (it != s_handler_map.end())
	{
		ReqHandler handle_func = it->second;
		handle_func(url, http_req->body.buf, http_req->body.len, connection, &HttpServer::SendHttpRsp);
		return;
	}
	// 其他请求
	if (route_check(http_req, "/")) // index page
	{
		SendHttpRsp(connection, html_index.c_str(), html_index.length(), Content_html);
		return;
	}
	else if (route_check(http_req, "/api/hello")) 
	{
		// 直接回传
		SendHttpRsp(connection, "welcome to httpserver", 21, Content_html);
	}
	else
	{
		SendHttpRsp(connection, html_404.c_str(), html_404.length(), Content_404);
	}
}
std::string HttpServer::html_index = R"(
		<!DOCTYPE html>
		<html lang="zh-CN">
		<head>
			<meta charset="UTF-8">
			<meta name="viewport" content="width=device-width, initial-scale=1.0">
			<title>首页</title>
			<style>
				body { 
					display: flex; 
					justify-content: center; 
					align-items: center; 
					height: 100vh; 
					background-color: #f0f0f0; 
					margin: 0; 
					font-family: Arial, sans-serif;
					color: #87CEEB; /* 淡蓝色字体 */
				}
				.container {
					text-align: center;
					max-width: 800px;
					margin: auto;
				}
				.api-section {
					display: flex;
					flex-direction: column;
					align-items: left;
					text-align: left;
				}
				.info-text {
					color: #000000; /* 黑色字体 */
					font-size: 1.5em;
					margin-bottom: 20px;
				}
				ul { 
					padding: 0;
					list-style-type: none;
					margin: 0;
				}
				li {
					margin: 10px 0;
					font-size: 1.5em;
				}
				a { 
					text-decoration: none; 
					color: #1E90FF; /* 深蓝色链接 */
				}
				a:hover {
					text-decoration: underline;
				}
			</style>
		</head>
		<body>
			<div class="container">
				<h1>欢迎</h1>
				<div class="api-section">
					<p class="info-text">以下是支持的API功能及其对应的URL：</p>
					<ul>
						<li><a href="/api/demo1">示例1 - /api/demo1</a></li>
						<li><a href="/api/hello">示例2 - /api/hello</a></li>
						<li><a href="/api/demo3">示例3 - /api/demo3</a></li>
					</ul>
				</div>
			</div>
		</body>
		</html>
		)";

std::string HttpServer::html_404= R"(
            <!DOCTYPE html>
            <html lang="zh-CN">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>404 Not Found</title>
                <style>
                    body { 
                        display: flex; 
                        justify-content: center; 
                        align-items: center; 
                        height: 100vh; 
                        background-color: #f0f0f0; 
                        margin: 0; 
                        font-family: Arial, sans-serif;
                    }
                    .container {
                        text-align: center;
                    }
                    h1 { 
                        font-size: 5em; 
                        color: #ff6f61; 
                    }
                    p { 
                        font-size: 1.5em; 
                        color: #333; 
                    }
                    a { 
                        text-decoration: none; 
                        color: #ff6f61; 
                        font-weight: bold;
                    }
                    a:hover {
                        text-decoration: underline;
                    }
                </style>
            </head>
            <body>
                <div class="container">
                    <h1>404</h1>
                    <p>错误！您当前使用页面/接口不存在</p>
                    <p><a href="/">返回首页</a></p>
                </div>
            </body>
            </html>
        )";