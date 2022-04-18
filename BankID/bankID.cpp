#include "bankID.h"


//Initlize the bankID varibles
static nlohmann::json json_auth		= nlohmann::json::object();
nlohmann::json bankID::credentials	= nlohmann::json::object();
time_t bankID::start_time_ = 0;
std::string readBuffer;

CURL* bankID::curl					= nullptr;
struct curl_slist* headers			= nullptr;
bool bankID::authenticated			= false;
bool bankID::initialized			= false;

//Initlize the qrCode varibles
QrToPng* bankID::qrCode::qrcode		= (QrToPng*)malloc(sizeof(QrToPng));
bool completed = false;

unsigned char* calcHmacSHA256(std::string_view decodedKey, std::string_view msg) {
	unsigned char* hash = (unsigned char*)calloc(EVP_MAX_MD_SIZE, sizeof(char));
	unsigned int hashLen;

	HMAC(EVP_sha256(), decodedKey.data(), static_cast<int>(decodedKey.size()),
		reinterpret_cast<unsigned char const*>(msg.data()), static_cast<int>(msg.size()), hash, &hashLen);

	return hash;
}


namespace bankID {

	size_t write_callback(char* contents, size_t size, size_t nmemb, void* userp){
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
		CERT(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback));
		CERT(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer));
		
		//Set Url, it might be updated! Also make sure to not use self signed certificates
		CERT(curl_easy_setopt(curl, CURLOPT_URL, "https://appapi2.test.bankid.com/rp/v5.1/auth"));
		CERT(curl_easy_setopt(curl, CURLOPT_VERBOSE, false));
		CERT(curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true));

		//Set header
		headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		initialized = true;
	}

	void auth(std::string personal_number) {
		if (!initialized) init();
		
		//Verify all the certificates for usage
		CERT(curl_easy_setopt(curl, CURLOPT_CAINFO,  "Resources/certificate.pem")); // local file
		CERT(curl_easy_setopt(curl, CURLOPT_SSLCERT, "Resources/FPT.pem"));		  // local file
		CERT(curl_easy_setopt(curl, CURLOPT_SSLKEY,  "Resources/key.pem"));         // local file

		//Prepare post data
		nlohmann::json json_temp;
		//if (!personal_number.empty()) json_temp = { {"endUserIp", end_user_ip}, {"personalNumber",personal_number}};
		if(!personal_number.empty()) json_temp = { {"endUserIp", end_user_ip} };
		
		json_temp["requirement"]["tokenStartRequired"] = true;
		//Set post data
		std::string data = json_temp.dump();

		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str()));
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length()));
		CERT(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));
		CERT(curl_easy_perform(curl));

		qrCode::start_time(); //Start time for animated qr code

		json_auth = json_auth.parse(readBuffer);
		authenticated = true;

	}

	void collect(std::string personal_number) {
		//Will not work if not authenticated
		if (!authenticated) return;

		nlohmann::json jason_temp;
		std::string temp;

		try {
			jason_temp = { {"orderRef", json_auth["orderRef"]} };
			temp = jason_temp.dump();
		}
		catch (std::exception& e) {std::cerr << e.what() << std::endl;}

		//Set url and post data for collecting information
		CERT(curl_easy_setopt(curl, CURLOPT_URL, "https://appapi2.test.bankid.com/rp/v5.1/collect"));
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDS, temp.c_str()));
		CERT(curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(temp.c_str())));
		CERT(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));

		//Block until data is collected
		jason_temp = 0;
		if(!completed) {
			try {
				CERT(curl_easy_perform(curl));
				jason_temp = nlohmann::json::parse(readBuffer.c_str());
				//According to the docs, every 1 sec 
				if (!std::string(jason_temp["status"]).compare("complete")) completed = true;
				if (!std::string(jason_temp["status"]).compare("pending"))  completed = false;
				if (!std::string(jason_temp["status"]).compare("failed")) { cancel(); completed = false; }
			}
			catch (std::exception& e) {std::cerr << e.what() << std::endl;}
		}

		if (completed) {
			credentials = jason_temp;
			completed = false;
		}
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
		json_auth.clear();

		//restart
		init();
		auth("202203072380");
	}

	void clean_up() {
		curl_easy_cleanup(curl);
	}

	namespace qrCode {
		QrToPng generate_qr_code(const char* data) {
			QrToPng png = QrToPng("qrCode.png", imgSize, minModulePixelSize, data, true, qrcodegen::QrCode::Ecc::LOW);
			if (png.writeToPNG()) {
				return png;
			}
			else
				std::cerr << "Failed..." << std::endl;
			return png;
		}
		
		void start_time() {
			start_time_ = time(NULL);
		}

		//Bankid documentation want qr code to update every 1 sec
		void update_time() {
			if (!authenticated) return;
				
			try {
				const auto temp = (long long)(time(NULL) - (long long)start_time_); // Convert to seconds
				std::stringstream ss;
				ss << "bankid." << json_auth["qrStartToken"].get<std::string>() << "." << temp << ".";
				unsigned char* c = calcHmacSHA256(json_auth["qrStartSecret"].get<std::string>(), std::to_string(temp));

				for (int i = 0; i < strlen((const char*)c); i++) {ss << std::hex << (int)c[i]; } // Convert to hex

				generate_qr_code(ss.str().c_str());
			}

			catch (std::exception& e) {std::cerr << e.what() << std::endl;}
		}
		
	}

}