#pragma once
#include <iostream>
#include <cassert>
#include <atomic>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include  <time.h>
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
#include "QrToPng.h"

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
	size_t write_callback(char* contents, size_t size, size_t nmemb, void* userp);

	std::string get_hintcode_pending(); //TODO
	std::string get_hindcode_failed();  //TODO

	namespace qrCode {
		extern QrToPng*	qrcode;
		
		extern std::chrono::steady_clock::time_point current_time;
		extern std::chrono::steady_clock::time_point last_time;
		extern int									 times;

		static const	 std::string fileName	= "qrCode.png";
		static const int imgSize			= 300;
		static const int minModulePixelSize = 1;

		QrToPng generate_qr_code(const char* data);
		void start_time();
		void update_time();
	}

}