/**
 * @file get_ip.hpp
 * @brief Functions for retrieving the public IP address.
 */
#pragma once

#include <string>

namespace { // Anonymous namespace to limit the scope of the functions to this file
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
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
}

/**
 * Retrieves the public IP address using the ipify API.
 * 
 * @return The public IP address as a string.
 */
std::string getPublicIP();