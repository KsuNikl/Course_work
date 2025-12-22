#pragma once
#include <string>

std::string random_salt_hex(std::size_t bytes = 16);
std::string sha256_hex(const std::string& data);
std::string pbkdf2_sha256_hex(const std::string& password, const std::string& salt, int iterations = 150000, std::size_t dklen = 32);
