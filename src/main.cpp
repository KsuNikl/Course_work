#include <QApplication>
#include <QStyleFactory>
#include "mainwindow.h"
#include "logindialog.h"
#include "db.hpp"
#include <QMessageBox>

QString getStyleSheet() {
    return R"(
        QWidget {
            background-color: #36393f; /* Discord Dark Gray */
            color: #dcddde;            /* Discord Text */
            font-family: "Segoe UI", "Roboto", "Helvetica", sans-serif;
            font-size: 14px;
        }
        QPushButton {
            background-color: #5865F2; /* Discord Blurple */
            color: white;
            border-radius: 4px;
            padding: 8px 16px;
            font-weight: bold;
            border: none;
        }
        QPushButton:hover {
            background-color: #4752C4;
        }
        QPushButton:pressed {
            background-color: #3c45a5;
        }
        QPushButton:disabled {
            background-color: #4f545c;
            color: #72767d;
        }
        QLineEdit {
            background-color: #202225;
            color: white;
            border: 1px solid #202225;
            border-radius: 4px;
            padding: 8px;
            selection-background-color: #5865F2;
        }
        QLineEdit:focus {
            border: 1px solid #5865F2;
        }
        QTableView {
            background-color: #2f3136;
            gridline-color: #202225;
            border: none;
            selection-background-color: #5865F2;
            selection-color: white;
        }
        QHeaderView::section {
            background-color: #202225;
            color: #b9bbbe;
            padding: 5px;
            border: none;
            font-weight: bold;
        }
        QTableCornerButton::section {
            background-color: #202225;
            border: none;
        }
        QTabWidget::pane {
            border: 1px solid #202225;
            background-color: #36393f;
        }
        QTabBar::tab {
            background: #2f3136;
            color: #b9bbbe;
            padding: 10px 20px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background: #36393f;
            color: white;
            border-bottom: 2px solid #5865F2;
        }
        QTabBar::tab:hover {
            background: #202225;
        }
        QScrollBar:vertical {
            background: #2f3136;
            width: 10px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #202225;
            min-height: 20px;
            border-radius: 5px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QTextEdit {
            background-color: #2f3136;
            border: none;
            border-radius: 4px;
            padding: 5px;
        }
        QLabel {
            color: #dcddde;
        }
    )";
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet(getStyleSheet());
    const char* db_host_env = std::getenv("DB_HOST");
    std::string db_host = db_host_env ? db_host_env : "localhost";
    std::string conn_str = "host=" + db_host + " port=5432 dbname=bdu user=bdu_app password=bdu_pass";
    try {
        Db db(conn_str);
        LoginDialog loginDlg(db);
        if (loginDlg.exec() != QDialog::Accepted) {
            return 0;
        }
        User user = loginDlg.getUser();
        MainWindow w(db, user);
        w.show();
        return app.exec();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Database Error", e.what());
        return 1;
    }
}