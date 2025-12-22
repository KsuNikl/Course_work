#include "crypto.hpp"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <vector>
#include <stdexcept>

static std::string to_hex(const unsigned char* data, std::size_t len) {
    std::ostringstream oss;
    oss << std::hex;
    for (std::size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return oss.str();
}

std::string sha256_hex(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    return to_hex(hash, SHA256_DIGEST_LENGTH);
}

std::string random_salt_hex(std::size_t bytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<unsigned char> buf(bytes);
    for (std::size_t i = 0; i < bytes; ++i) buf[i] = static_cast<unsigned char>(dist(gen));
    return to_hex(buf.data(), buf.size());
}

std::string pbkdf2_sha256_hex(const std::string& password, const std::string& salt, int iterations, std::size_t dklen)
{
    if (iterations <= 0) iterations = 150000;
    if (dklen == 0) dklen = 32;
    std::vector<unsigned char> out(dklen);
    const unsigned char* salt_data = reinterpret_cast<const unsigned char*>(salt.data());
    int salt_len = static_cast<int>(salt.size());
    int res = PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()), salt_data, salt_len, iterations, EVP_sha256(), static_cast<int>(dklen), out.data());
    if (res != 1) {
        unsigned long err = ERR_get_error();
        char buf[256] = {0};
        ERR_error_string_n(err, buf, sizeof(buf));
        throw std::runtime_error(std::string("PBKDF2 failure: ") + buf);
    }
    return to_hex(out.data(), out.size());
}
