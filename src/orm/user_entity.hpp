struct UserEntity {
  long long id;
  std::string login;
  std::string email;          // может быть пустой
  std::string password_hash;
  std::string salt;
  bool is_admin;
  std::string role;

  static UserEntity from_row(const pqxx::row& r);
};
