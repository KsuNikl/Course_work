#include "addthreatdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <regex>

AddThreatDialog::AddThreatDialog(Db& db, long userId, QWidget *parent)
    : QDialog(parent), m_db(db), m_repo(db), m_userId(userId), added_code_()
{
    setWindowTitle("Добавить угрозу вручную");
    resize(600, 480);

    QVBoxLayout *main = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();
    edtCode = new QLineEdit();
    edtTitle = new QLineEdit();
    edtDescription = new QTextEdit();
    edtDescription->setFixedHeight(120);
    edtConsequences = new QTextEdit();
    edtConsequences->setFixedHeight(100);
    edtSource = new QLineEdit();

    form->addRow("Код (УБИ.001):", edtCode);
    form->addRow("Название:", edtTitle);
    form->addRow("Описание:", edtDescription);
    form->addRow("Последствия:", edtConsequences);
    form->addRow("Источник:", edtSource);

    main->addLayout(form);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    QPushButton *btnOk = new QPushButton("Добавить");
    QPushButton *btnCancel = new QPushButton("Отмена");
    btnOk->setFixedHeight(36);
    btnCancel->setFixedHeight(36);
    buttons->addWidget(btnCancel);
    buttons->addWidget(btnOk);
    main->addLayout(buttons);

    connect(btnOk, &QPushButton::clicked, this, &AddThreatDialog::onOk);
    connect(btnCancel, &QPushButton::clicked, this, &AddThreatDialog::reject);
}

void AddThreatDialog::onOk() {
    QString code_q = edtCode->text().trimmed();
    QString title_q = edtTitle->text().trimmed();

    if (code_q.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Код угрозы не может быть пустым.");
        return;
    }
    if (title_q.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Название не может быть пустым.");
        return;
    }

    static const std::regex code_re(R"(^(?:(?:УБИ|UBI)\.)\d{1,6}$)", std::regex::icase);
    std::string code = code_q.toStdString();
    if (!std::regex_match(code, code_re)) {
        QMessageBox::StandardButton rb = QMessageBox::question(this, "Подтвердите код",
            "Код не соответствует формату УБИ.### или UBI.###. Всё равно сохранить?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (rb == QMessageBox::No) return;
    }

    Threat t;
    t.code = code;
    t.title = title_q.toStdString();
    t.description = edtDescription->toPlainText().toStdString();
    t.consequences = edtConsequences->toPlainText().toStdString();
    t.source = edtSource->text().toStdString();

    std::string err;
    bool ok = false;
    try {
        ok = m_repo.insert_threat(t, m_userId, err);
    } catch (const std::exception &e) {
        err = e.what();
    }

    if (ok) {
        added_code_ = t.code;
        QMessageBox::information(this, "Успех", "Угроза добавлена.");
        accept();
        return;
    } else {
        if (err.empty()) err = "Неизвестная ошибка при добавлении угрозы.";
        QMessageBox::critical(this, "Ошибка", QString::fromStdString(err));
        return;
    }
}
