/**
 * @file get_ip.cpp
 * @brief Implementation file for the get_ip function.
 */
#include "get_ip.hpp"

#include <curl/curl.h>

/**
 * @brief Callback function used for writing data received from a network request.
 * 
 * This function is used as a callback for writing data received from a network request
 * into a string buffer. It appends the received data to the provided string buffer.
 * 
 * @param contents A pointer to the received data.
 * @param size The size of each element in the received data.
 * @param nmemb The number of elements in the received data.
 * @param userp A pointer to a string buffer where the received data will be appended.
 * @return The total number of bytes written.
 */
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/**
 * Retrieves the public IP address using the ipify API.
 * 
 * @return The public IP address as a string.
 */
std::string getPublicIP() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}