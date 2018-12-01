#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dir/include/CkCrypt2.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <QThread>
#include <queue>
#include <future>

const size_t buffer_mas_length = 128000;

main_window::main_window(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(60, 100, 1600, 600);

    QStringList headers = {"Files", "Size", "Delete"};
    ui->treeWidget->setColumnCount(3);
    ui->treeWidget->setShowGrid(true);
    ui->treeWidget->setHorizontalHeaderLabels(headers);
    ui->treeWidget->horizontalHeader()->setStretchLastSection(true);


    QCommonStyle style;

    ui->actionScan_Directory->setIcon(QIcon("../dirdemo/dir/Icons-Land-Vista-Hardware-Devices-Computer.ico"));
    ui->actionDelete_Files->setIcon(QIcon("../dirdemo/dir/Recycle_Bin_Empty.ico"));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));


    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionDelete_Files, &QAction::triggered, this, &main_window::delete_selected);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
}

main_window::~main_window() {}


void main_window::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    QFileInfo check_file(dir);
    if (check_file.exists()) {
        std::map<std::string, std::vector<QFileInfo>> hashes;
#ifdef DEBUG
        freopen("output.txt","w",stdout);
#endif
        QDir dira = QDir(dir);

        std::map<int64_t, std::vector<QFileInfo>> same_size;

        std::vector<std::vector<std::vector<QFileInfo>::size_type>> equal_files;

        // make_groups(dira, &same_size);
        auto args = std::bind(&main_window::make_groups, dira, &same_size);
        auto handle = std::thread(args);

        //        make_groups(dira, &same_size);

        int row_number = 0;
        bool thread_end = false;
        uint64_t eq_files_groups = 0;
        uint32_t used_groups = 0;
        std::queue<std::pair<QFileInfo, bool>> out_queue;
        std::mutex mutex;
        handle.join();

        bool ready = true;
        auto args1 = std::bind(&main_window::check_files_eq,
                               &same_size,
                               &equal_files,
                               &thread_end,
                               &eq_files_groups,
                               &used_groups,
                               &out_queue,
                               &ready,
                               &mutex
                               );

        auto handle1 = std::thread(args1);


        while (!thread_end){
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            mutex.lock();
            if (!out_queue.empty()){
                auto elem = out_queue.front();
                auto elem1 = elem.first;
                out_queue.pop();
                mutex.unlock();
                QTreeWidgetItem *item1 = new QTreeWidgetItem(Qt::CheckStateRole);
                item1->setCheckState(Qt::CheckState(false));
                ui->treeWidget->insertRow(row_number);
//                ui->treeWidget->
                ui->treeWidget->setItem(row_number, 0, new QTableWidgetItem(elem1.filePath()));
                ui->treeWidget->setItem(row_number, 1, new QTableWidgetItem(QString::number(elem1.size())));
                ui->treeWidget->setItem(row_number, 2, item1);
                //                if (elem.second){}
                for (int j = 0; j < 3; j++){
                    ui->treeWidget->item(row_number, j)->setBackgroundColor(elem.second ? "white" : "grey");
                }
            }
            mutex.unlock();
            ui->treeWidget->resizeColumnsToContents();
        }
        handle1.join();
        while (!out_queue.empty()){
            auto elem = out_queue.front();
            auto elem1 = elem.first;
            out_queue.pop();
            QTableWidgetItem *item1 = new QTableWidgetItem(Qt::CheckStateRole);
            item1->setCheckState(Qt::CheckState(false));
            ui->treeWidget->insertRow(row_number);
            ui->treeWidget->setItem(row_number, 0, new QTableWidgetItem(elem1.filePath()));
            ui->treeWidget->setItem(row_number, 1, new QTableWidgetItem(QString::number(elem1.size())));
            ui->treeWidget->setItem(row_number, 2, item1);
            for (int j = 0; j < 3; j++){
                ui->treeWidget->item(row_number, j)->setBackgroundColor(elem.second ? "white" : "grey");
            }
        }
        ui->treeWidget->resizeColumnsToContents();
    } else {
        return;
    }
}


void main_window::check_files_eq(std::map<int64_t, std::vector<QFileInfo>> *array,
                                 std::vector<std::vector<std::vector<QFileInfo>::size_type>> *equal_files,
                                 bool * thread_end,
                                 uint64_t *eq_files_groups,
                                 uint32_t * used_groups,
                                 std::queue<std::pair<QFileInfo, bool>> *do_out, bool *ready, std::mutex * mutex) {
    (*used_groups) = 0;
    for (auto &elem : *array) {
#ifdef DEBUG
        std::cout << "doing new group" << std::endl;
#endif
        *eq_files_groups = (*equal_files).size();
        if (elem.second.size() > 1) {
            if (elem.first != 0){
                std::vector<std::unique_ptr<std::ifstream>> filesin;
                filesin.reserve(elem.second.size());
                std::vector<std::string> mas_strings;
                for (std::vector<std::unique_ptr<std::ifstream>>::size_type i = 0; i < elem.second.size(); i++) {

#ifdef DEBUG
                    mas_strings.push_back(elem.second[i].absoluteFilePath().toStdString());
                    std::cout << "open file -> " << mas_strings.back() << std::endl;
#endif
                    std::unique_ptr<std::ifstream> temp(
                                new std::ifstream(elem.second[i].absoluteFilePath().toStdString(), std::ifstream::binary));
                    (filesin).push_back(std::move(temp));
                }
                std::vector<bool> used_files(elem.second.size());

                char buffer1[buffer_mas_length];
                char buffer2[buffer_mas_length];

                //                std::vector<std::vector<std::vector<QFileInfo>::size_type>> equal_files;
                bool flag_eq = false;
                for (std::vector<std::ifstream *>::size_type i = 0; i < filesin.size(); i++) {
                    if (used_files[i]) {
                        continue;
                    }
                    used_files[i] = true;

                    bool added = false;
                    flag_eq = true;

                    for (std::vector<std::ifstream *>::size_type j = i + 1; j < (filesin).size(); j++) {
                        while (!(*filesin[i]).eof()) {
                            if (!flag_eq) {
                                break;
                            }
                            (*filesin[i]).read((char *) buffer1, buffer_mas_length * sizeof(char));
                            (*filesin[j]).read((char *) buffer2, buffer_mas_length * sizeof(char));
                            for (std::streamsize k = 0; k < (*filesin[i]).gcount(); k++) {
                                if (buffer1[k] != buffer2[k]) {
                                    flag_eq = false;
                                    break;
                                }
                            }
                        }
                        if (flag_eq) {
                            if (!added) {
                                (*equal_files).push_back({i});
                            }
                            added = true;
                            (*equal_files).back().push_back(j);
                            used_files[j] = true;
                        }
                        flag_eq = true;
                    }
                }
                for (std::vector<std::ifstream *>::size_type i = 0; i < filesin.size(); i++) {
                    (*filesin[i]).close();
#ifdef DEBUG
                    std::cout << "close file -> " << mas_strings[i] << std::endl;
#endif
                }
                *ready = false;
                for (uint64_t h = *eq_files_groups ; h < (*equal_files).size(); h++){
                    for (std::vector<std::vector<QFileInfo>::size_type>::size_type i = 0; i < (*equal_files)[h].size(); i++) {
                        mutex->lock();
                        (*do_out).push(std::make_pair(elem.second[i], (*used_groups) % 2 == 0));
                        mutex->unlock();
                    }
                    ++(*used_groups);
                }
                *ready = true;
            }else{
                for (uint64_t h = 0 ; h < elem.second.size(); h++){
                    mutex->lock();
                    (*do_out).push(std::make_pair(elem.second[h], (*used_groups) % 2 == 0));
                    mutex->unlock();
                }
                ++(*used_groups);
            }
        }
    }
    *thread_end = true;
}

void main_window::make_groups(QDir &dir, std::map<int64_t, std::vector<QFileInfo>> *array) {
    QFileInfoList list = dir.entryInfoList();
    for (QFileInfo file_info : list) {
#ifdef DEBUG
        std::cout << "Doing groups in -> " << file_info.absoluteFilePath().toStdString() << std::endl;
#endif
        if (file_info.fileName() == "." || file_info.fileName() == "..") {
            continue;
        }
        if (file_info.isDir()) {
            dir.cd(file_info.absoluteFilePath());
            make_groups(dir, array);
            dir.cdUp();
        }
        if (file_info.isFile()) {
            file_info.size();
            if (array->find(file_info.size()) != array->end()) {
                (*array)[file_info.size()].push_back(file_info);
            } else {
                std::vector<QFileInfo> temp;
                temp.push_back(file_info);
                (*array).insert(std::make_pair(file_info.size(), temp));
            }
        }
    }
}


void main_window::do_hash(QDir &dir, std::map<std::string, std::vector<QFileInfo>> *hashes) {
    QFileInfoList list = dir.entryInfoList();
    for (QFileInfo file_info : list) {
        if (file_info.fileName() == "." || file_info.fileName() == "..") {
            continue;
        }
        if (file_info.isDir()) {
#ifdef DEBUG
            std::cout << "going into: " << file_info.absoluteFilePath().toStdString() << std::endl;
#endif
            dir.cd(file_info.absoluteFilePath());
            do_hash(dir, hashes);
            dir.cdUp();
        }
        if (file_info.isFile()) {
#ifdef DEBUG
            std::cout << "doing file: " << file_info.absoluteFilePath().toStdString() << std::endl;
#endif
            std::string hash;
            hash_it(file_info.absoluteFilePath(), &hash);
            if (hash == "") {
                continue;
            }
            auto iter = (*hashes).find(hash);
            if (iter == (*hashes).end()) {
                (*hashes).emplace(std::make_pair(hash, std::vector(1, file_info)));
            } else {
                iter->second.push_back(file_info);
            }
        }
    }

}

void main_window::delete_selected() {
    for (int i = 0; i < ui->treeWidget->rowCount(); i++) {
        if (ui->treeWidget->item(i, 2)->checkState()) {
            QString file = ui->treeWidget->takeItem(i, 0)->text();
            QFile(file).remove();
            ui->treeWidget->removeRow(i--);
        }
    }
}

void main_window::hash_it(QString const &str, std::string *out_string) {
    CkCrypt2 crypt;
    crypt.UnlockComponent("Hello");
    crypt.put_HashAlgorithm("sha256");
    crypt.put_EncodingMode("hex");

    (*out_string) = std::move(crypt.hashFileENC(str.toStdString().c_str()));
    if (crypt.get_LastMethodSuccess() != true) {
        std::cout << "file hash error" << str.toStdString() << std::endl;
        std::cout << crypt.lastErrorText() << std::endl;
        (*out_string) = std::string();
        return;
    }
}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
