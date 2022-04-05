#pragma once
#include <iostream>
#include <curl/curl.h>
#include "nlohmann/json.hpp"
#include <cassert>

#define END_USER_IP "194.168.2.25"
#define CERT(x) if(x!=CURLE_OK)return;

namespace bankID {
	extern bool authenticated;
	extern bool initialized;
	extern CURL* curl;
	extern nlohmann::json credentials;
	static const std::string end_user_ip = "194.168.2.25";
	
	void init();
	void auth(std::string personal_number);
	void collect(std::string personal_number);
	void cancel();
	void clean_up();
	size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp);

	std::string get_hintcode_pending();
	std::string get_hindcode_failed();

}