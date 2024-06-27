/**
 * @file hash_SHA256.hpp
 * @brief Header file for the SHA256 hash function.
 */
#include <string>


/**
 * Calculates the SHA256 hash of a given string.
 *
 * @param str The input string to calculate the hash for.
 * @return The SHA256 hash of the input string.
 */
std::string sha256(const std::string str);