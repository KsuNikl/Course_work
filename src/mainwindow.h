#pragma once
#include <QMainWindow>
#include <QStandardItemModel>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QTextEdit>
#include <QLineEdit>
#include <QTabWidget>
#include "db.hpp"
#include "auth.hpp"
#include "threats.hpp"
#include "search_session.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(Db& db, const User& user, QWidget *parent = nullptr);

private slots:
    void onSearch();
    void onAddToReport();
    void onExportReport();
    void onUpdateDatabase();
    void onRefreshHistory();
    void onAddManualThreat();
    void onManageUsers();
    void onLogout();

private:
    Db& m_db;
    User m_user;
    ThreatRepository m_threats;
    SearchSession m_session;
    QTabWidget* m_tabs = nullptr;
    QLineEdit* searchEdit = nullptr;
    QTableView* searchTable = nullptr;
    QTableView* reportTable = nullptr;
    QTableView* historyTable = nullptr;
    QTextEdit* logArea = nullptr;
    QProgressBar* progressBar = nullptr;
    QLabel* userInfo = nullptr;
    QPushButton* btnAddManual = nullptr;
    QPushButton* btnLogout = nullptr;
    QWidget* adminTab = nullptr;
    QPushButton* btnManageUsers = nullptr;
    QStandardItemModel* searchModel = nullptr;
    QStandardItemModel* reportModel = nullptr;
    QStandardItemModel* historyModel = nullptr;
    std::vector<Threat> currentSearchResults;
    void setupUi();
    void log(const QString& msg);
    static bool is_code_like(const std::string& s);
    static std::string lower_copy(const std::string& s);
};
