#include "bankID.h"

static nlohmann::json json_auth		= nlohmann::json::object();;
nlohmann::json bankID::credentials	= nlohmann::json::object();

std::string readBuffer				= "";
CURL* bankID::curl					= nullptr;
struct curl_slist* headers			= nullptr;
bool bankID::authenticated			= false;
bool bankID::initialized			= false;

namespace bankID {

	size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp){
		((std::string*)userp)->clear();
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}

	void init() {
		// initlize for windows
		CERT(curl_global_init(CURL_GLOBAL_ALL));
		curl = curl_easy_init();
		assert(curl);

		//Set callback function when readbuffer is changed
		CERT(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback));
		CERT(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer));

		//Set Url, it might be updated! Also make sure to not use self signed certificates
		CERT(curl_easy_setopt(curl, CURLOPT_URL, "https://appapi2.test.bankid.com/rp/v5.1/auth"));
		CERT(curl_easy_setopt(curl, CURLOPT_VERBOSE, true));
		CERT(curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true));

		//Set header
		headers = curl_slist_append(headers, "Content-Type: application/json");
		initialized = true;
	}

	void auth(std::string personal_number) {
		if (!initialized) init();
		
		//Verify all the certificates for usage
		CERT(curl_easy_setopt(curl, CURLOPT_CAINFO,  "certificate.pem")); // local
		CERT(curl_easy_setopt(curl, CURLOPT_SSLCERT, "FPT.pem"));		  // local
		CERT(curl_easy_setopt(curl, CURLOPT_SSLKEY,  "key.pem"));         // local

		//Prepare post data
		nlohmann::json json_temp;
		if (!personal_number.empty()) json_temp = { {"endUserIp", end_user_ip}, {"personalNumber",personal_number} };
		if (personal_number.empty())  json_temp = { {"endUserIp", end_user_ip} };
		
		//Set post data
		std::string data = json_temp.dump();
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str()));
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length()));
		CERT(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));
		CERT(curl_easy_perform(curl));
		
		json_auth = json_auth.parse(readBuffer);
		authenticated = true;

	}

	void collect(std::string personal_number) {
		//Will not work if not authenticated
		if (!authenticated) return;
		
		nlohmann::json jason_temp = { {"orderRef", json_auth["orderRef"]} };
		std::string temp = jason_temp.dump();
		
		//Set url and post data for collecting information
		CERT(curl_easy_setopt(curl, CURLOPT_URL, "https://appapi2.test.bankid.com/rp/v5.1/collect"));
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDS, temp.c_str()));
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(temp.c_str())));
		CERT(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));

		//Block until data is collected
		bool completed = false;
		while (completed == false) {
			CERT(curl_easy_perform(curl));
			jason_temp = nlohmann::json::parse(readBuffer.c_str());
			//According to the docs, every 2 sec 
			Sleep(2000);
			if (!std::string(jason_temp["status"]).compare("complete")) completed = true;
			if (!std::string(jason_temp["status"]).compare("pending"))  completed = false;
			if (!std::string(jason_temp["status"]).compare("failed")) { cancel(); completed = true; }
		}

		credentials = jason_temp;
	}

	void cancel() {
		nlohmann::json jason_temp = { {"orderRef", json_auth["orderRef"]} };
		std::string temp = jason_temp.dump();

		//Set url and post data for cancel
		CERT(curl_easy_setopt(curl, CURLOPT_URL, "https://appapi2.test.bankid.com/rp/v5.1/cancel"));
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDS, temp.c_str()));
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(temp.c_str())));
		CERT(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));

		CERT(curl_easy_perform(curl));
	}

	void clean_up() {
		curl_easy_cleanup(curl);
	}

}