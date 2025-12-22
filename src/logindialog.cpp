#include "logindialog.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>

LoginDialog::LoginDialog(Db& db, QWidget *parent) 
    : QDialog(parent), m_db(db), m_auth(db) 
{
    setWindowTitle("Вход в систему");
    resize(360, 420);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->setContentsMargins(40, 40, 40, 40);
    QLabel *lblTitle = new QLabel("Welcome Back!", this);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: white; margin-bottom: 10px;");
    layout->addWidget(lblTitle);
    QLabel *lblSub = new QLabel("База угроз ФСТЭК", this);
    lblSub->setAlignment(Qt::AlignCenter);
    lblSub->setStyleSheet("color: #b9bbbe; margin-bottom: 20px;");
    layout->addWidget(lblSub);
    edtLogin = new QLineEdit(this);
    edtLogin->setPlaceholderText("Логин");
    edtLogin->setFixedHeight(40);
    layout->addWidget(edtLogin);
    edtPass = new QLineEdit(this);
    edtPass->setPlaceholderText("Пароль");
    edtPass->setEchoMode(QLineEdit::Password);
    edtPass->setFixedHeight(40);
    layout->addWidget(edtPass);
    layout->addStretch();
    QPushButton *btnLogin = new QPushButton("Войти", this);
    btnLogin->setFixedHeight(45);
    btnLogin->setCursor(Qt::PointingHandCursor);
    layout->addWidget(btnLogin);
    QPushButton *btnReg = new QPushButton("Нет аккаунта? Регистрация", this);
    btnReg->setStyleSheet("background-color: transparent; color: #00aff4; border: none;");
    btnReg->setCursor(Qt::PointingHandCursor);
    layout->addWidget(btnReg);
    connect(btnLogin, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(btnReg, &QPushButton::clicked, this, &LoginDialog::onRegister);
}

void LoginDialog::onLogin() {
    std::string err;
    std::string login = edtLogin->text().toStdString();
    std::string pass = edtPass->text().toStdString();
    if (m_auth.login_user(login, pass, m_user, err)) {
        accept();
    } else {
        QMessageBox::warning(this, "Ошибка входа", QString::fromStdString(err));
    }
}

void LoginDialog::onRegister() {
    bool ok;
    QString login = QInputDialog::getText(this, "Регистрация", "Придумайте логин:", QLineEdit::Normal, "", &ok);
    if (!ok || login.isEmpty()) return;
    QString email = QInputDialog::getText(this, "Регистрация", "Email:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QString pass = QInputDialog::getText(this, "Регистрация", "Пароль:", QLineEdit::Password, "", &ok);
    if (!ok || pass.isEmpty()) return;
    std::string err;
    if (m_auth.register_user(login.toStdString(), email.toStdString(), pass.toStdString(), err)) {
        QMessageBox::information(this, "Успех", "Регистрация успешна! Теперь войдите с новым паролем.");
    } else {
        QMessageBox::warning(this, "Ошибка регистрации", QString::fromStdString(err));
    }
}