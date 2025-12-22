#pragma once
#include <QDialog>
#include <QTableView>
#include <QStandardItemModel>
#include "db.hpp"

class ManageUsersDialog : public QDialog {
    Q_OBJECT

public:
    explicit ManageUsersDialog(Db& db, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onSetRole();

private:
    Db& m_db;
    QTableView *m_table;
    QStandardItemModel *m_model;
    void loadUsers();
};
