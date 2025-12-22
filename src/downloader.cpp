#include "downloader.hpp"
#include <curl/curl.h>
#include <cstdio>
#include <iostream>

#ifdef _WIN32
 #include <windows.h>
#else
 #include <unistd.h>
#endif

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* stream) {
    return std::fwrite(ptr, size, nmemb, static_cast<FILE*>(stream));
}

static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    auto* cb = static_cast<std::function<void(double, double)>*>(clientp);
    if (cb && *cb) {
        (*cb)(static_cast<double>(dlnow), static_cast<double>(dltotal));
    }
    return 0; 
}

static void sleep_1s() {
#ifdef _WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
}

bool download_https(const std::string& url, const std::string& filepath, std::string& error, std::function<void(double, double)> progress_cb)
{
    error.clear();
    CURL* curl = curl_easy_init();
    if (!curl) {
        error = "curl_easy_init failed";
        return false;
    }
    FILE* fp = std::fopen(filepath.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        error = "cannot open file for write: " + filepath;
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1800L);
    curl_easy_setopt(curl, CURLOPT_PROXY, "");
    if (progress_cb) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_cb);
    } else {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    }
    CURLcode res = CURLE_OK;
    long http_code = 0;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        std::rewind(fp); 
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (res == CURLE_OK && http_code >= 200 && http_code < 300) {
            break;
        }
        std::cerr << "Download attempt " << attempt << " failed: " << curl_easy_strerror(res) << " (HTTP " << http_code << ")" << std::endl;
        if (attempt < 3) sleep_1s();
    }
    std::fclose(fp);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        error = std::string("curl error: ") + curl_easy_strerror(res);
        return false;
    }
    if (http_code < 200 || http_code >= 300) {
        error = "HTTP error code: " + std::to_string(http_code);
        return false;
    }
    return true;
}