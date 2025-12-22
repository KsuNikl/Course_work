#pragma once
#include <string>

class Db;

struct User {
    long long id = 0;
    std::string login;
    std::string email;
    std::string role = "viewer";
    bool is_admin() const {return role == "admin";}
    bool can_add() const {return role == "admin" || role == "editor";}
    bool can_read() const {return true;}
};

class AuthService {
public:
    explicit AuthService(Db& db);
    bool register_user(const std::string& login, const std::string& email, const std::string& password, std::string& error);
    bool login_user(const std::string& login, const std::string& password, User& out_user, std::string& error) const;

private:
    Db& db_;
    bool is_password_strong(const std::string& password, std::string& error) const;
    bool user_exists(const std::string& login) const;
    bool is_first_user() const;
};