#include "manageusersdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <pqxx/pqxx>
#include <QHeaderView>

ManageUsersDialog::ManageUsersDialog(Db& db, QWidget *parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle("Управление пользователями");
    resize(600, 400);
    QVBoxLayout *main = new QVBoxLayout(this);
    m_table = new QTableView();
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"ID", "Login", "Role", "is_admin"});
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    main->addWidget(m_table);
    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *btnRefresh = new QPushButton("Обновить");
    QPushButton *btnSetRole = new QPushButton("Изменить роль");
    QPushButton *btnClose = new QPushButton("Закрыть");
    buttons->addWidget(btnRefresh);
    buttons->addWidget(btnSetRole);
    buttons->addStretch();
    buttons->addWidget(btnClose);
    main->addLayout(buttons);
    connect(btnRefresh, &QPushButton::clicked, this, &ManageUsersDialog::onRefresh);
    connect(btnSetRole, &QPushButton::clicked, this, &ManageUsersDialog::onSetRole);
    connect(btnClose, &QPushButton::clicked, this, &ManageUsersDialog::accept);
    loadUsers();
}

void ManageUsersDialog::loadUsers() {
    m_model->removeRows(0, m_model->rowCount());
    try {
        pqxx::work tx(m_db.conn());
        auto r = tx.exec("SELECT id, login, role, is_admin FROM users ORDER BY id");
        tx.commit();
        for (const auto &row : r) {
            QList<QStandardItem*> items;
            QStandardItem *itId = new QStandardItem(QString::number(row["id"].as<long long>()));
            QStandardItem *itLogin = new QStandardItem(row["login"].c_str());
            QStandardItem *itRole = new QStandardItem(row["role"].c_str());
            QStandardItem *itIsAdmin = new QStandardItem(row["is_admin"].as<bool>() ? "true" : "false");
            items << itId << itLogin << itRole << itIsAdmin;
            m_model->appendRow(items);
        }
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Ошибка", e.what());
    }
}

void ManageUsersDialog::onRefresh() {
    loadUsers();
}

void ManageUsersDialog::onSetRole() {
    QModelIndexList sel = m_table->selectionModel()->selectedRows();
    if (sel.empty()) {
        QMessageBox::information(this, "Info", "Выберите пользователя.");
        return;
    }
    int row = sel.first().row();
    QString login = m_model->item(row, 1)->text();
    QString curRole = m_model->item(row, 2)->text();
    QStringList roles = {"admin", "editor", "viewer"};
    bool ok = false;
    QString chosen = QInputDialog::getItem(this, "Изменить роль", QString("Выберите роль для %1").arg(login), roles, roles.indexOf(curRole), false, &ok);
    if (!ok) return;
    try {
        pqxx::work tx(m_db.conn());
        tx.exec_params("UPDATE users SET role = $1, is_admin = $2 WHERE login = $3", chosen.toStdString(), (chosen == "admin"), login.toStdString());
        tx.commit();
        QMessageBox::information(this, "OK", "Роль обновлена.");
        loadUsers();
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Ошибка", e.what());
    }
}
