#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include "threats.hpp"
#include "db.hpp"

class AddThreatDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddThreatDialog(Db& db, long userId, QWidget *parent = nullptr);
    std::string added_code() const { return added_code_; }

private slots:
    void onOk();

private:
    Db& m_db;
    ThreatRepository m_repo;
    long m_userId;

    QLineEdit *edtCode;
    QLineEdit *edtTitle;
    QLineEdit *edtSource;
    QTextEdit *edtDescription;
    QTextEdit *edtConsequences;

    std::string added_code_;
};
