#pragma once
#include <pqxx/pqxx>
#include <optional>
#include <string>
#include <vector>

class Db;

// Результат для аутентификации (то, что нужно AuthService)
struct UserAuthRecord {
    long long id = 0;
    std::string login;
    std::string email;
    std::string password_hash;
    std::string salt;
    bool is_admin = false;
    std::string role;
};

// Строка для списка пользователей (ManageUsersDialog)
struct UserListRow {
    long long id = 0;
    std::string login;
    std::string role;
    bool is_admin = false;
};

class UserRepository {
public:
    explicit UserRepository(Db& db);

    bool exists_by_login(pqxx::transaction_base& tx, const std::string& login) const;
    long long count_users(pqxx::transaction_base& tx) const;

    std::optional<UserAuthRecord> get_auth_by_login(pqxx::transaction_base& tx, const std::string& login) const;

    void insert_user(
        pqxx::transaction_base& tx,
        const std::string& login,
        const std::string& email,
        const std::string& password_hash,
        const std::string& salt,
        bool is_admin,
        const std::string& role
    ) const;

    std::vector<UserListRow> list_users(pqxx::transaction_base& tx) const;

    void update_role_admin_by_login(
        pqxx::transaction_base& tx,
        const std::string& login,
        const std::string& role,
        bool is_admin
    ) const;

private:
    Db& db_;
};
