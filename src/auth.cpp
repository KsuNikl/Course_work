#include "auth.hpp"
#include "db.hpp"
#include "crypto.hpp"
#include <pqxx/pqxx>
#include <cctype>
#include <regex>
#include <algorithm>
#include <sstream>

AuthService::AuthService(Db& db) : db_(db) {}

bool AuthService::is_password_strong(const std::string& password, std::string& error) const {
    if (password.size() < 8) {
        error = "Пароль должен быть не короче 8 символов.";
        return false;
    }
    bool has_letter = false, has_digit = false, has_special = false;
    for (unsigned char ch : password) {
        if (std::isalpha(ch)) has_letter = true;
        else if (std::isdigit(ch)) has_digit = true;
        else has_special = true;
    }
    if (!has_letter || !has_digit || !has_special) {
        error = "Пароль должен содержать буквы, цифры и спецсимвол.";
        return false;
    }
    return true;
}

bool AuthService::user_exists(const std::string& login) const {
    pqxx::work tx(db_.conn());
    auto r = tx.exec_params("SELECT 1 FROM users WHERE login = $1", login);
    tx.commit();
    return !r.empty();
}

bool AuthService::is_first_user() const {
    pqxx::work tx(db_.conn());
    auto r = tx.exec("SELECT COUNT(*) AS c FROM users");
    tx.commit();
    return r[0]["c"].as<long long>() == 0;
}

static std::string trim_copy(const std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && std::isspace((unsigned char)s[i])) ++i;
    while (j > i && std::isspace((unsigned char)s[j-1])) --j;
    return s.substr(i, j - i);
}

static std::string to_lower_copy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return std::tolower(c); });
    return out;
}

static bool is_email_valid(const std::string& email) {
    static const std::regex re(R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)");
    return std::regex_match(email, re);
}

bool AuthService::register_user(const std::string& login, const std::string& email_in, const std::string& password, std::string& error) {
    if (login.empty()) { error = "Логин не должен быть пустым."; return false; }
    std::string email = trim_copy(email_in);
    if (email.empty() || !is_email_valid(email)) { error = "Некорректный email."; return false; }
    if (!is_password_strong(password, error)) return false;
    if (user_exists(login)) { error = "Пользователь с таким логином уже существует."; return false; }
    const std::string salt = random_salt_hex(16);
    const int iterations = 150000;
    const std::size_t dklen = 32;
    std::string hash;
    try {
        hash = pbkdf2_sha256_hex(password, salt, iterations, dklen);
    } catch (const std::exception &e) {
        error = std::string("Ошибка хеширования пароля: ") + e.what();
        return false;
    }
    std::ostringstream stored;
    stored << "pbkdf2_sha256$" << iterations << "$" << salt << "$" << hash;
    const std::string stored_hash = stored.str();
    const bool first = is_first_user(); // первый пользователь — админ
    const std::string role = first ? "admin" : "viewer";
    try {
        pqxx::work tx(db_.conn());
        tx.exec_params(
            "INSERT INTO users(login, email, password_hash, salt, is_admin, role) VALUES($1,$2,$3,$4,$5,$6)",
            login, email, stored_hash, salt, first, role
        );
        tx.commit();
    } catch (const std::exception &e) {
        error = std::string("Ошибка базы данных: ") + e.what();
        return false;
    }
    return true;
}

bool AuthService::login_user(const std::string& login, const std::string& password, User& out_user, std::string& error) const {
    pqxx::work tx(db_.conn());
    auto r = tx.exec_params(
        "SELECT id, login, email, password_hash, salt, is_admin, role FROM users WHERE login = $1",
        login
    );
    tx.commit();
    if (r.empty()) { error = "Неверный логин или пароль."; return false; }
    const std::string salt = r[0]["salt"].c_str();
    const std::string stored = r[0]["password_hash"].c_str();
    bool ok = false;
    try {
        const std::string prefix = "pbkdf2_sha256$";
        if (stored.rfind(prefix, 0) == 0) {
            size_t pos = prefix.size();
            size_t p1 = stored.find('$', pos);
            if (p1 != std::string::npos) {
                std::string iter_str = stored.substr(pos, p1 - pos);
                size_t p2 = stored.find('$', p1 + 1);
                if (p2 != std::string::npos) {
                    std::string salt_stored = stored.substr(p1 + 1, p2 - (p1 + 1));
                    std::string hash_stored = stored.substr(p2 + 1);
                    int iterations = std::stoi(iter_str);
                    std::string calc_hash = pbkdf2_sha256_hex(password, salt_stored, iterations, 32);
                    if (calc_hash == hash_stored) ok = true;
                }
            }
        } else {
            std::string calc = sha256_hex(password + salt);
            if (calc == stored) ok = true;
        }
    } catch (const std::exception &e) {
        error = "Ошибка аутентификации.";
        return false;
    }
    if (!ok) { error = "Неверный логин или пароль."; return false; }
    out_user.id = r[0]["id"].as<long long>();
    out_user.login = r[0]["login"].c_str();
    out_user.email = r[0]["email"].c_str();
    try {
        out_user.role = r[0]["role"].as<std::string>();
        if (out_user.role.empty()) out_user.role = r[0]["is_admin"].as<bool>() ? "admin" : "viewer";
    } catch (...) {
        out_user.role = r[0]["is_admin"].as<bool>() ? "admin" : "viewer";
    }
    return true;
}
