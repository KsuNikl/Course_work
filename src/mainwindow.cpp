#include "mainwindow.h"
#include "downloader.hpp"
#include "xlsx_converter.hpp"
#include "importer.hpp"
#include "logindialog.h"
#include "addthreatdialog.h"
#include "manageusersdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableView>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextEdit>
#include <QApplication>
#include <QProgressBar>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QCoreApplication>
#include <thread>
#include <QMetaObject>
#include <fstream>
#include <pqxx/pqxx>
#include <regex>
#include <algorithm>
#include <cstdlib>
using std::string;

static string to_lower_copy_impl(const string& s) {
    string r = s;
    std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c){ return std::tolower(c); });
    return r;
}

bool MainWindow::is_code_like(const std::string& s) {
    static const std::regex re(R"(^(?:(?:УБИ|UBI)\.)\d{1,6}$)", std::regex::icase);
    return std::regex_match(s, re);
}

std::string MainWindow::lower_copy(const std::string& s) {
    return to_lower_copy_impl(s);
}

MainWindow::MainWindow(Db& db, const User& user, QWidget *parent)
    : QMainWindow(parent), m_db(db), m_user(user), m_threats(db)
{
    setupUi();
    log("Добро пожаловать, " + QString::fromStdString(user.login));
}

void MainWindow::setupUi() {
    resize(1000, 700);
    setWindowTitle("Threat Intelligence Platform");
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    QWidget *header = new QWidget();
    header->setStyleSheet("background-color: #202225; border-bottom: 1px solid #101113;");
    header->setFixedHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    QLabel *logo = new QLabel("BDU Client", header);
    logo->setStyleSheet("font-size: 18px; font-weight: bold; color: white; border: none;");
    userInfo = new QLabel(QString::fromStdString(m_user.login) + (m_user.is_admin() ? " [ADMIN]" : ""), header);
    userInfo->setStyleSheet("color: #b9bbbe; font-size: 14px; border: none;");
    userInfo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    btnLogout = new QPushButton("Выйти", header);
    btnLogout->setFixedHeight(36);
    btnLogout->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(logo);
    headerLayout->addStretch();
    headerLayout->addWidget(userInfo);
    headerLayout->addWidget(btnLogout);
    mainLayout->addWidget(header);
    m_tabs = new QTabWidget(this);
    m_tabs->setStyleSheet("border: none;");
    mainLayout->addWidget(m_tabs);
    QWidget *searchTab = new QWidget();
    QVBoxLayout *searchLayout = new QVBoxLayout(searchTab);
    searchLayout->setContentsMargins(20, 20, 20, 20);
    searchLayout->setSpacing(15);
    QHBoxLayout *searchBar = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Поиск по базе (CVE, название, описание)...");
    searchEdit->setFixedHeight(40);
    QPushButton *btnSearch = new QPushButton("Найти");
    btnSearch->setFixedHeight(40);
    btnSearch->setCursor(Qt::PointingHandCursor);
    searchBar->addWidget(searchEdit);
    searchBar->addWidget(btnSearch);
    searchTable = new QTableView();
    searchModel = new QStandardItemModel(this);
    searchModel->setHorizontalHeaderLabels({"Код", "Название", "Описание"});
    searchTable->setModel(searchModel);
    searchTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    searchTable->verticalHeader()->setVisible(false);
    searchTable->setAlternatingRowColors(true);
    QPushButton *btnAdd = new QPushButton("Добавить в отчет");
    btnAdd->setCursor(Qt::PointingHandCursor);
    searchLayout->addLayout(searchBar);
    searchLayout->addWidget(searchTable);
    searchLayout->addWidget(btnAdd);
    m_tabs->addTab(searchTab, "Поиск");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::onAddToReport);
    connect(searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearch);
    connect(btnLogout, &QPushButton::clicked, this, &MainWindow::onLogout);
    QWidget *reportTab = new QWidget();
    QVBoxLayout *reportLayout = new QVBoxLayout(reportTab);
    reportLayout->setContentsMargins(20, 20, 20, 20);
    reportTable = new QTableView();
    reportModel = new QStandardItemModel(this);
    reportModel->setHorizontalHeaderLabels({"Код", "Название"});
    reportTable->setModel(reportModel);
    reportTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    reportTable->verticalHeader()->setVisible(false);
    QPushButton *btnExport = new QPushButton("💾 Экспорт в CSV");
    btnExport->setStyleSheet("background-color: #3ba55c;");
    btnExport->setCursor(Qt::PointingHandCursor);
    reportLayout->addWidget(new QLabel("Сформированный отчет:"));
    reportLayout->addWidget(reportTable);
    reportLayout->addWidget(btnExport);
    m_tabs->addTab(reportTab, "Отчет");
    connect(btnExport, &QPushButton::clicked, this, &MainWindow::onExportReport);
    //админ
    adminTab = new QWidget();
    QVBoxLayout *adminLayout = new QVBoxLayout(adminTab);
    adminLayout->setContentsMargins(20, 20, 20, 20);
    adminLayout->setSpacing(15);
    btnAddManual = new QPushButton("Добавить угрозу вручную");
    btnAddManual->setFixedHeight(44);
    btnAddManual->setCursor(Qt::PointingHandCursor);
    btnAddManual->setVisible(m_user.can_add());
    QPushButton *btnUpdate = new QPushButton("Обновить базу данных (FSTEC)");
    btnUpdate->setFixedHeight(50);
    btnUpdate->setStyleSheet("background-color: #FAA61A; color: black; font-weight: bold;");
    btnUpdate->setCursor(Qt::PointingHandCursor);
    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setVisible(false);
    progressBar->setStyleSheet("QProgressBar { border: 2px solid #202225; border-radius: 5px; text-align: center; color: white; } QProgressBar::chunk { background-color: #5865F2; width: 20px; }");
    historyTable = new QTableView();
    historyModel = new QStandardItemModel(this);
    historyModel->setHorizontalHeaderLabels({"Дата", "Админ", "Доб.", "Обн.", "Источник"});
    historyTable->setModel(historyModel);
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    historyTable->verticalHeader()->setVisible(false);
    QPushButton *btnRefresh = new QPushButton("Обновить историю");
    btnManageUsers = new QPushButton("Управление пользователями");
    btnManageUsers->setFixedHeight(44);
    btnManageUsers->setCursor(Qt::PointingHandCursor);
    btnManageUsers->setVisible(m_user.is_admin());
    adminLayout->addWidget(new QLabel("<h3>Управление данными</h3>"));
    adminLayout->addWidget(btnUpdate);
    adminLayout->addWidget(progressBar);
    adminLayout->addSpacing(20);
    adminLayout->addWidget(new QLabel("<h3>История обновлений</h3>"));
    adminLayout->addWidget(historyTable);
    adminLayout->addWidget(btnRefresh);
    adminLayout->addSpacing(10);
    QHBoxLayout *adminButtons = new QHBoxLayout();
    adminButtons->addWidget(btnAddManual);
    adminButtons->addStretch();
    adminButtons->addWidget(btnManageUsers);
    adminLayout->addLayout(adminButtons);
    connect(btnUpdate, &QPushButton::clicked, this, &MainWindow::onUpdateDatabase);
    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshHistory);
    connect(btnManageUsers, &QPushButton::clicked, this, &MainWindow::onManageUsers);
    connect(btnAddManual, &QPushButton::clicked, this, &MainWindow::onAddManualThreat);
    m_tabs->addTab(adminTab, m_user.is_admin() ? "Администратор" : (lower_copy(m_user.role) == "editor" ? "Редактор" : "Администратор"));
    int adminIdx = m_tabs->indexOf(adminTab);
    if (!(m_user.is_admin() || lower_copy(m_user.role) == "editor")) {
        m_tabs->setTabVisible(adminIdx, false);
    } else {
        m_tabs->setTabVisible(adminIdx, true);
    }
    QWidget *footer = new QWidget();
    footer->setFixedHeight(120);
    footer->setStyleSheet("background-color: #202225; padding: 10px;");
    QVBoxLayout *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(10, 5, 10, 5);
    logArea = new QTextEdit();
    logArea->setReadOnly(true);
    logArea->setStyleSheet("background-color: #101113; color: #00ff00; font-family: Consolas, monospace; border: none;");
    footerLayout->addWidget(new QLabel("System Log:", footer));
    footerLayout->addWidget(logArea);
    mainLayout->addWidget(footer);
}

void MainWindow::log(const QString& msg) {
    if (!logArea) return;
    logArea->append("> " + msg);
    QTextCursor c = logArea->textCursor();
    c.movePosition(QTextCursor::End);
    logArea->setTextCursor(c);
}

void MainWindow::onSearch() {
    std::string keyword = searchEdit->text().toStdString();
    if (keyword.empty()) return;
    if (is_code_like(keyword)) {
        auto ot = m_threats.get_by_code(keyword);
        searchModel->removeRows(0, searchModel->rowCount());
        currentSearchResults.clear();
        if (ot) {
            currentSearchResults.push_back(*ot);
            QList<QStandardItem*> row;
            row << new QStandardItem(QString::fromStdString(ot->code));
            row << new QStandardItem(QString::fromStdString(ot->title));
            row << new QStandardItem(QString::fromStdString(ot->description));
            searchModel->appendRow(row);
            log(QString("Найдена угроза по коду: %1").arg(QString::fromStdString(ot->code)));
        } else {
            log("Угроза с таким кодом не найдена.");
            QMessageBox::information(this, "Результат", "Угроза с таким кодом не найдена.");
        }
        return;
    }
    currentSearchResults = m_threats.search_by_keyword(keyword);
    searchModel->removeRows(0, searchModel->rowCount());
    for (const auto& t : currentSearchResults) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::fromStdString(t.code));
        row << new QStandardItem(QString::fromStdString(t.title));
        row << new QStandardItem(QString::fromStdString(t.description));
        searchModel->appendRow(row);
    }
    log(QString("Найдено угроз: %1").arg((int)currentSearchResults.size()));
}

void MainWindow::onAddToReport() {
    int count = m_session.add(currentSearchResults);
    reportModel->removeRows(0, reportModel->rowCount());
    log(QString("Добавлено в отчет новых: %1").arg(count));
    for(const auto& t : currentSearchResults) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::fromStdString(t.code));
        row << new QStandardItem(QString::fromStdString(t.title));
        reportModel->appendRow(row);
    }
}

void MainWindow::onExportReport() {
    if (m_session.size() == 0) {
        QMessageBox::information(this, "Info", "Отчет пуст");
        return;
    }
    const char* envExport = std::getenv("EXPORT_DIR");
    QString exportDir;
    if (envExport && envExport[0] != '\0') {
        exportDir = QString::fromLocal8Bit(envExport);
    } else {
        QString appDir = QCoreApplication::applicationDirPath();
        exportDir = QDir(appDir).filePath("../results");
    }
    QDir dir(exportDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            QMessageBox::warning(this, "Ошибка", "Не удалось создать папку для экспорта: " + exportDir);
            return;
        }
    }
    QString defaultName = QString("report_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString defaultPath = dir.filePath(defaultName);
    QString path = QFileDialog::getSaveFileName(this, "Сохранить отчет", defaultPath, "CSV Files (*.csv)");
    if (path.isEmpty()) return;
    std::string err;
    if (m_session.export_csv(path.toStdString(), err)) {
        log("Экспорт успешен: " + path);
        QMessageBox::information(this, "Success", "Файл сохранен: " + path);
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } else {
        log("Ошибка экспорта: " + QString::fromStdString(err));
        QMessageBox::critical(this, "Error", QString::fromStdString(err));
    }
}

void MainWindow::onUpdateDatabase() {
    log("Запуск фонового процесса обновления...");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (progressBar) {
        progressBar->setValue(0);
        progressBar->setVisible(true);
    }
    const char* db_host_env = std::getenv("DB_HOST");
    std::string db_host = db_host_env ? db_host_env : "localhost";
    std::string conn_str = "host=" + db_host + " port=5432 dbname=bdu user=bdu_app password=bdu_pass";
    long userId = m_user.id;
    std::thread([this, conn_str, userId]() {
        std::string err;
        const std::string url = "https://bdu.fstec.ru/files/documents/thrlist.xlsx";
        const std::string xlsx_path = "data/thrlist.xlsx";
        const std::string csv_path = "data/thrlist.csv";
        auto logToGui = [this](QString msg) {
            QMetaObject::invokeMethod(this, [this, msg](){ log(msg); });
        };
        auto progressToGui = [this](double dl, double total) {
            if (total > 0 && progressBar) {
                int percent = static_cast<int>((dl / total) * 100.0);
                QMetaObject::invokeMethod(this, [this, percent](){ progressBar->setValue(percent); });
            }
        };
        #ifdef _WIN32
            system("mkdir data >nul 2>nul");
        #else
            system("mkdir -p data >/dev/null 2>/dev/null");
        #endif
        logToGui("Попытка скачивания файла...");
        bool downloadSuccess = download_https(url, xlsx_path, err, progressToGui);
        if (!downloadSuccess) {
            std::ifstream f(xlsx_path);
            if (f.good()) {
                logToGui("Интернет-скачивание не удалось (" + QString::fromStdString(err) + ").");
                logToGui("Найден ЛОКАЛЬНЫЙ файл thrlist.xlsx. Используем его.");
                QMetaObject::invokeMethod(this, [this](){ if (progressBar) progressBar->setValue(100); });
            } else {
                logToGui("ОШИБКА скачивания: " + QString::fromStdString(err));
                logToGui("Локальный файл тоже не найден.");
                QMetaObject::invokeMethod(this, [this](){ QApplication::restoreOverrideCursor(); if (progressBar) progressBar->setVisible(false); });
                return;
            }
        } else {
            logToGui("Скачано успешно.");
            QMetaObject::invokeMethod(this, [this](){ if (progressBar) progressBar->setValue(100); });
        }
        logToGui("Конвертация XLSX -> CSV (подождите)...");
        if (!convert_xlsx_to_csv(xlsx_path, csv_path, err)) {
            logToGui("Ошибка конвертации: " + QString::fromStdString(err));
            QMetaObject::invokeMethod(this, [this](){ QApplication::restoreOverrideCursor(); if (progressBar) progressBar->setVisible(false); });
            return;
        }
        logToGui("Конвертация завершена.");
        logToGui("Импорт в базу...");
        try {
            Db localDb(conn_str);
            ImportStats stats;
            if (!import_threats_csv(localDb, csv_path, userId, stats, err)) {
                logToGui("Ошибка импорта: " + QString::fromStdString(err));
                QMetaObject::invokeMethod(this, [this](){ QApplication::restoreOverrideCursor(); if (progressBar) progressBar->setVisible(false); });
                return;
            }
            try {
                pqxx::work tx(localDb.conn());
                tx.exec_params("INSERT INTO update_log(user_id, inserted, updated, source) VALUES($1,$2,$3,$4)", userId, stats.inserted, stats.updated, std::string("fstec"));
                tx.commit();
            } catch (const std::exception &e) {
                logToGui("Не удалось записать update_log: " + QString::fromStdString(e.what()));
            }
            logToGui(QString("Импорт завершен. Вставлено: %1, Обновлено: %2").arg(stats.inserted).arg(stats.updated));
        } catch (const std::exception &e) {
            logToGui("Ошибка работы с БД: " + QString::fromStdString(e.what()));
        }
        QMetaObject::invokeMethod(this, [this](){ QApplication::restoreOverrideCursor(); if (progressBar) progressBar->setVisible(false); });
    }).detach();
}

void MainWindow::onRefreshHistory() {
    if (!historyModel) return;
    historyModel->removeRows(0, historyModel->rowCount());
    try {
        pqxx::work tx(m_db.conn());
        auto r = tx.exec("SELECT imported_at, user_id, inserted, updated, source FROM update_log ORDER BY imported_at DESC LIMIT 100");
        tx.commit();
        for (const auto &row : r) {
            QList<QStandardItem*> items;
            QString date = QString::fromStdString(row["imported_at"].c_str());
            QStandardItem *itDate = new QStandardItem(date);
            QStandardItem *itUser = new QStandardItem(QString::number(row["user_id"].as<long long>()));
            QStandardItem *itIns = new QStandardItem(QString::number(row["inserted"].as<int>()));
            QStandardItem *itUpd = new QStandardItem(QString::number(row["updated"].as<int>()));
            QStandardItem *itSrc = new QStandardItem(row["source"].c_str());
            items << itDate << itUser << itIns << itUpd << itSrc;
            historyModel->appendRow(items);
        }
        log("История обновлений обновлена.");
    } catch (const std::exception &e) {
        log(QString("Ошибка при получении истории: %1").arg(e.what()));
    }
}

void MainWindow::onAddManualThreat() {
    if (!m_user.can_add()) {
        QMessageBox::warning(this, "Нет прав", "У вас нет прав добавлять угрозы.");
        return;
    }
    AddThreatDialog dlg(m_db, m_user.id, this);
    if (dlg.exec() == QDialog::Accepted) {
        std::string added_code = dlg.added_code();
        if (!added_code.empty()) {
            auto ot = m_threats.get_by_code(added_code);
            if (ot) {
                currentSearchResults.clear();
                currentSearchResults.push_back(*ot);
                searchModel->removeRows(0, searchModel->rowCount());
                QList<QStandardItem*> row;
                row << new QStandardItem(QString::fromStdString(ot->code));
                row << new QStandardItem(QString::fromStdString(ot->title));
                row << new QStandardItem(QString::fromStdString(ot->description));
                searchModel->appendRow(row);
                log(QString("Добавлена и найдена угроза: %1").arg(QString::fromStdString(ot->code)));
            } else {
                log("Угроза добавлена, но не найдена по коду.");
            }
        } else {
            log("Угроза добавлена.");
        }
        m_session = SearchSession();
        reportModel->removeRows(0, reportModel->rowCount());
    }
}

void MainWindow::onManageUsers() {
    if (!m_user.is_admin()) {
        QMessageBox::warning(this, "Нет прав", "Только администратор может управлять пользователями.");
        return;
    }
    ManageUsersDialog dlg(m_db, this);
    dlg.exec();
}

void MainWindow::onLogout() {
    this->hide();
    LoginDialog dlg(m_db);
    if (dlg.exec() != QDialog::Accepted) {
        this->show();
        return;
    }
    User newUser = dlg.getUser();
    m_user = newUser;
    if (userInfo) userInfo->setText(QString::fromStdString(m_user.login) + (m_user.is_admin() ? " [ADMIN]" : ""));
    if (btnAddManual) btnAddManual->setVisible(m_user.can_add());
    if (btnManageUsers) btnManageUsers->setVisible(m_user.is_admin());
    if (m_tabs && adminTab) {
        int idx = m_tabs->indexOf(adminTab);
        if (idx >= 0) {
            bool shouldShow = m_user.is_admin() || (lower_copy(m_user.role) == "editor");
            m_tabs->setTabVisible(idx, shouldShow);
            QString label = m_user.is_admin() ? "Администратор" : (lower_copy(m_user.role) == "editor" ? "Редактор" : "Администратор");
            m_tabs->setTabText(idx, label);
        }
    }
    m_session = SearchSession();
    currentSearchResults.clear();
    if (searchModel) searchModel->removeRows(0, searchModel->rowCount());
    if (reportModel) reportModel->removeRows(0, reportModel->rowCount());
    this->show();
    log(QString("Вошёл новый пользователь: %1").arg(QString::fromStdString(m_user.login)));
}
