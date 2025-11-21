#include <curl/curl.h>
#include "nlohmann/json.hpp" // 引入nlohmann/json库


// 回调函数：接收服务器响应  (回调函数格式恒定)
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
	size_t newLength = size * nmemb;
	try {
		s->append((char*)contents, newLength);
	}
	catch (std::bad_alloc& e) {
		return 0; // 内存分配失败
	}
	return newLength;
}

/**
 * 发送尺寸和体积数据到指定URL
 * @param length 长度(mm)
 * @param width 宽度(mm)
 * @param height 高度(mm)
 * @param cuboid_volume 外接长方体体积(立方米)
 * @param actual_volume 实际物体体积(立方米)
 * @param target_url 目标服务器URL
 * @return 是否发送成功
 */

bool sendDataToServer(double length, double width, double height,
	double cuboid_volume, double actual_volume,
	const std::string& target_url) {
	// 1. 初始化libcurl
	CURL* curl = curl_easy_init();
	if (!curl) {
		std::cerr << "libcurl初始化失败！" << std::endl;
		return false;
	}

	// 2. 构造JSON数据
	nlohmann::json data;
	data["length"] = length;               // 长(mm)
	data["width"] = width;                 // 宽(mm)
	data["height"] = height;               // 高(mm)
	data["cuboid_volume"] = cuboid_volume; // 外接长方体体积(立方米)
	data["actual_volume"] = actual_volume; // 实际物体体积(立方米)

	// 增加时间戳（便于服务器记录时间）
	time_t now = time(nullptr);
	struct tm timeinfo;
	localtime_s(&timeinfo, &now); // Windows用localtime_s，Linux/macOS用localtime_r
	char time_str[20];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
	data["timestamp"] = time_str;

	std::string json_str = data.dump(); // 转换JSON为字符串

	// 3. 配置HTTP请求
	curl_easy_setopt(curl, CURLOPT_URL, target_url.c_str()); // 目标URL
	curl_easy_setopt(curl, CURLOPT_POST, 1L); // 使用POST方法
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str()); // POST数据

	// 设置请求头（声明数据格式为JSON）
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	// 配置响应处理
	std::string response;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	// 设置超时（连接5秒，总超时10秒）
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	// 4. 执行请求并判断结果
	CURLcode res = curl_easy_perform(curl);
	bool success = false;
	if (res == CURLE_OK) {
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code == 200) {
			std::cout << "数据发送成功！服务器响应：" << response << std::endl;
			success = true;
		}
		else {
			std::cerr << "发送失败，服务器状态码：" << response_code << std::endl;
		}
	}
	else {
		std::cerr << "请求失败：" << curl_easy_strerror(res) << std::endl;
	}

	// 5. 清理资源
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return success;
}

int main(int argc, char** argv)
{
  		bool result = sendDataToServer(
			finalLength, finalWidth, max_depth, finalLength * finalWidth * max_depth / 1e9, volumeUnicom / 1e9,
			EnvCa.target_url  // 指向本地监听的端口
		);
}
