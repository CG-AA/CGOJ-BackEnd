#pragma once
#include <curl/curl.h>
#include <string>

class sand_box_api {
    std::string url;
    int port;
    std::string token;
    CURL* curl;
