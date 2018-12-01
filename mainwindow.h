#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "filedigger.h"

#include <QMainWindow>
#include <memory>
#include <iostream>
#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>
#include <vector>
#include <map>
#include <queue>
#include <QTreeWidget>
#include <thread>

#include <bits/std_mutex.h>


namespace Ui {
class MainWindow;
}

class main_window : public QMainWindow
{
    Q_OBJECT



public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();
private:



private slots:
    void show_data(QTreeWidgetItem *data_out);

    void update_status(int);

    void update_status_range(int);

    void close_search();

    void stop_scanning();

    void select_directory();

    // void delete_selected();
//    void do_hash(QDir &, std::map<std::string, std::vector<QFileInfo>> *);
//    void hash_it(QString const&, std::string *);
    void show_about_dialog();

    void setup_tree();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    FileDigger *worker;
};

#endif // MAINWINDOW_H
