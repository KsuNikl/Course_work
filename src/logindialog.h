#pragma once
#include <QDialog>
#include <QLineEdit>
#include "auth.hpp"
#include "db.hpp"

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(Db& db, QWidget *parent = nullptr);
    User getUser() const { return m_user; }

private slots:
    void onLogin();
    void onRegister();

private:
    Db& m_db;
    AuthService m_auth;
    User m_user;
    QLineEdit *edtLogin;
    QLineEdit *edtPass;
};