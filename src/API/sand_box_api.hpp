#pragma once
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <mutex>

class sand_box_api {
    std::string full_url;
    std::string token;
    std::mutex mtx;

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s) {
        size_t newLength = size * nmemb;
        size_t oldLength = s->size();
        try {
            s->resize(oldLength + newLength);
        } catch (std::bad_alloc &e) {
            // Handle memory problem
            return 0;
        }
        std::copy((char*)contents, (char*)contents + newLength, s->begin() + oldLength);
        return size * nmemb;
    }

public:
    sand_box_api(const std::string& url, int port, const std::string& token) {
        full_url = url + ":" + std::to_string(port);
        this->token = token;
    }

    // Send POST w/ code && input to the sandbox
    std::string POST(const std::string& code, const std::string& input) {
        std::lock_guard<std::mutex> lock(mtx);
        CURL *curl;
        CURLcode res;
        std::string readBuffer;
        curl = curl_easy_init();
        if (curl) {
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
            headers = curl_slist_append(headers, ("Authorization: " + token).c_str());

            curl_easy_setopt(curl, CURLOPT_URL, (full_url + "/api/endpoint").c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ("code=" + code + "&input=" + input).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                readBuffer = "";
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
        return readBuffer;
    }
};