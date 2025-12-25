#include "user_repository.hpp"
#include "../db.hpp"

UserRepository::UserRepository(Db& db) : db_(db) {}

bool UserRepository::exists_by_login(pqxx::transaction_base& tx, const std::string& login) const {
    auto r = tx.exec_params("SELECT 1 FROM users WHERE login = $1", login);
    return !r.empty();
}

long long UserRepository::count_users(pqxx::transaction_base& tx) const {
    auto r = tx.exec("SELECT COUNT(*) AS c FROM users");
    return r[0]["c"].as<long long>();
}

std::optional<UserAuthRecord> UserRepository::get_auth_by_login(pqxx::transaction_base& tx, const std::string& login) const {
    auto r = tx.exec_params(
        "SELECT id, login, email, password_hash, salt, is_admin, role "
        "FROM users WHERE login = $1",
        login
    );
    if (r.empty()) return std::nullopt;

    UserAuthRecord out;
    out.id = r[0]["id"].as<long long>();
    out.login = r[0]["login"].c_str();
    out.email = r[0]["email"].c_str();
    out.password_hash = r[0]["password_hash"].c_str();
    out.salt = r[0]["salt"].c_str();
    out.is_admin = r[0]["is_admin"].as<bool>();
    try { out.role = r[0]["role"].as<std::string>(); } catch (...) { out.role = ""; }
    return out;
}

void UserRepository::insert_user(
    pqxx::transaction_base& tx,
    const std::string& login,
    const std::string& email,
    const std::string& password_hash,
    const std::string& salt,
    bool is_admin,
    const std::string& role
) const {
    tx.exec_params(
        "INSERT INTO users(login, email, password_hash, salt, is_admin, role) "
        "VALUES($1,$2,$3,$4,$5,$6)",
        login, email, password_hash, salt, is_admin, role
    );
}

std::vector<UserListRow> UserRepository::list_users(pqxx::transaction_base& tx) const {
    auto r = tx.exec("SELECT id, login, role, is_admin FROM users ORDER BY id");
    std::vector<UserListRow> out;
    out.reserve(r.size());
    for (const auto& row : r) {
        UserListRow u;
        u.id = row["id"].as<long long>();
        u.login = row["login"].c_str();
        try { u.role = row["role"].as<std::string>(); } catch (...) { u.role = ""; }
        u.is_admin = row["is_admin"].as<bool>();
        if (u.role.empty()) u.role = u.is_admin ? "admin" : "viewer";
        out.push_back(std::move(u));
    }
    return out;
}

void UserRepository::update_role_admin_by_login(
    pqxx::transaction_base& tx,
    const std::string& login,
    const std::string& role,
    bool is_admin
) const {
    tx.exec_params(
        "UPDATE users SET role = $1, is_admin = $2 WHERE login = $3",
        role, is_admin, login
    );
}
