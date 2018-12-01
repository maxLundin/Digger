#include "filedigger.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "dir/include/CkCrypt2.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <QThread>
#include <queue>
#include <future>
#include <QStatusBar>

main_window::main_window(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), worker(nullptr){
    ui->setupUi(this);
    setGeometry(60, 100, 1600, 600);

    QCommonStyle style;

    ui->actionScan_Directory->setIcon(QIcon("../Digger/dir/Icons-Land-Vista-Hardware-Devices-Computer.ico"));
    ui->actionDelete_Files->setIcon(QIcon("../Digger/dir/open_things.jpg"));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    ui->actionStopScanning->setIcon(style.standardIcon(QCommonStyle::SP_BrowserStop));

    ui->statusBar->addPermanentWidget(ui->label);
    ui->statusBar->addPermanentWidget(ui->progressBar);

    ui->progressBar->hide();
    ui->treeWidget->hide();
    ui->actionStopScanning->setVisible(false);


#ifdef DEBUG
   // freopen("output.txt","w",stdout);
#endif

    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
    connect(ui->actionDelete_Files, &QAction::triggered, ui->treeWidget, &tree_widget::open_everything);
    connect(ui->actionStopScanning, &QAction::triggered, this, &main_window::stop_scanning);
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem * , int)), ui->treeWidget,
            SLOT(on_tree_item_clicked(QTreeWidgetItem *)));
}

main_window::~main_window() {}

void main_window::stop_scanning(){
    worker->stop_scanning();
}

void main_window::setup_tree(){
    ui->treeWidget->show();
    ui->treeWidget->update();

    ui->treeWidget->setColumnCount(3);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeWidget->headerItem()->setText(0,"File Path");
    ui->treeWidget->headerItem()->setText(1,"File Name");
    ui->treeWidget->headerItem()->setText(2,"File Size");
}



void main_window::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    QFileInfo check_file(dir);
    ui->actionScan_Directory->setVisible(false);
    if (check_file.exists()) {


        ui->treeWidget->clear_tree();


        ui->actionStopScanning->setVisible(true);
        setup_tree();

        auto *thread = new QThread();
        worker = new FileDigger(dir);
        worker->moveToThread(thread);

        connect(thread, SIGNAL (started()), worker, SLOT (do_file_search()));
        connect(worker, SIGNAL (finished(int)), thread, SLOT (quit()));
        connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
        connect(worker, SIGNAL (ready_to_add(QTreeWidgetItem *)), this, SLOT(show_data(QTreeWidgetItem *)));
        connect(worker, SIGNAL (status(int)), this, SLOT(update_status(int)));
        connect(worker, SIGNAL (status_range(int)), this, SLOT(update_status_range(int)));
        connect(worker, SIGNAL (finished()), this, SLOT(close_search()));

        ui->label->setText("Scanning selected directory.");

        thread->start();
    } else {
        return;
    }
}

void main_window::close_search(){
    ui->progressBar->hide();
    ui->label->setText(std::move(QString("Finished all ").append(std::to_string(ui->progressBar->maximum()).data()).append(" files.")));
    ui->actionStopScanning->setVisible(false);
    ui->actionScan_Directory->setVisible(true);
}

void main_window::update_status_range(int maxRange){
    ui->progressBar->show();
    ui->label->show();
    ui->progressBar->setRange(0, maxRange);

    ui->label->setText(std::move(QString("files checked : 0 / ")
                                 .append(std::to_string(ui->progressBar->maximum()).data())));


}

void main_window::update_status(int value){
    ui->progressBar->setValue(value);

    ui->label->setText(std::move(QString("files checked : ")
                                 .append(std::to_string(value).data())
                                 .append(" / ")
                                 .append(std::to_string(ui->progressBar->maximum()).data())));
}

void main_window::show_data(QTreeWidgetItem *data_out){
#ifdef DEBUG
    std::cout << "showing data about group with size -> " << (data_out->text(2)).toStdString() << std::endl;
#endif
    ui->treeWidget->add_to_tree(std::move(data_out));
}


void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
