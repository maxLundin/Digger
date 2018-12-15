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

    void show_about_dialog();

    void setup_tree();

    void delete_all_dublicates();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    FileDigger * worker;
    QThread * thread;
};

#endif // MAINWINDOW_H
