#pragma once
#include <string>
#include <functional>

bool download_https(const std::string& url, const std::string& filepath, std::string& error, std::function<void(double, double)> progress_cb = nullptr);