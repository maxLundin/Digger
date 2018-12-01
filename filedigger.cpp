#include "filedigger.h"

#include <bits/unique_ptr.h>
#include <QDir>
#include <QTreeWidget>
#include <fstream>
#include <thread>


FileDigger::FileDigger(QString dir)
{
    working_dir = dir;
    is_scanning_now = false;
}

void FileDigger::check_files_eq(FileDigger * that,
                                std::map<int64_t, std::vector<QFileInfo>> *array,
                                bool * thread_end,
                                std::queue<std::pair<QFileInfo, bool> > *do_out,
                                std::mutex * mutex) {
    std::vector<std::vector<size_t>> equal_files;
    uint64_t eq_files_groups = 0;
    bool used_groups = true;
    int files_checked = 0;
    char buffer1[buffer_mas_length];
    char buffer2[buffer_mas_length];
    for (auto i = (*array).rbegin(); i != (*array).rend(); ++i){
        auto& elem = (*i);
#ifdef DEBUG
        std::cout << "doing new group" << std::endl;
#endif
        if (!that->is_scanning_now){
            return;
        }
        eq_files_groups = (equal_files).size();

        if (elem.second.size() > 1) {
            if (elem.first != 0){
                std::vector<std::unique_ptr<std::ifstream>> filesin;
                filesin.reserve(elem.second.size());
#ifdef DEBUG
                std::vector<std::string> mas_strings;
#endif

                for (std::vector<std::unique_ptr<std::ifstream>>::size_type i = 0; i < elem.second.size(); i++) {
#ifdef DEBUG
                    mas_strings.push_back(elem.second[i].absoluteFilePath().toStdString());
                    std::cout << "open file -> " << mas_strings.back() << std::endl;
#endif
                    std::unique_ptr<std::ifstream> temp;
                    temp.reset(new std::ifstream(elem.second[i].absoluteFilePath().toStdString(), std::ios::binary));
                    if(temp->is_open()){
                        (filesin).push_back(std::move(temp));
                    }else{
                        std::cout << "Error opening file -> " <<
                                     elem.second[i].absoluteFilePath().toStdString() << std::endl;
                        for (int _ = 0 ; _ < 3; _ ++){
                            std::cout << "Trying again -> " <<
                                         elem.second[i].absoluteFilePath().toStdString() << std::endl;
                            temp.reset(new std::ifstream(elem.second[i].absoluteFilePath().toStdString(), std::ios::binary));
                            if(temp->is_open()){
                                (filesin).push_back(std::move(temp));
                                std::cout << "It opened!!!" << std::endl;
                                break;
                            }else{
                                std::cout << "Error" << std::endl;
                            }
                        }
                    }

                }
                std::vector<bool> used_files(elem.second.size());

                bool flag_eq = false;
                for (std::vector<std::unique_ptr<std::ifstream>>::size_type i = 0; i < filesin.size(); i++) {
                    if (used_files[i]) {
                        continue;
                    }
                    used_files[i] = true;
                    bool added = false;
                    flag_eq = true;
                    for (std::vector<std::unique_ptr<std::ifstream>>::size_type j = i + 1; j < (filesin).size(); j++) {
                        if (used_files[j]){
                            continue;
                        }
                        (*filesin[i]).clear();
                        (*filesin[j]).clear();
                        (*filesin[i]).seekg(0, std::ios::beg);
                        (*filesin[j]).seekg(0, std::ios::beg);
                        while (!(*filesin[i]).eof() && flag_eq) {
                            (*filesin[i]).read(buffer1, buffer_mas_length * sizeof(char));
                            (*filesin[j]).read(buffer2, buffer_mas_length * sizeof(char));
                            std::streamsize buff_length = (*filesin[i]).gcount();
                            for (std::streamsize k = 0; flag_eq && k < buff_length; k++) {
                                flag_eq = (buffer1[k] == buffer2[k]);
                            }
                        }
                        if (flag_eq) {
                            if (!added) {
                                (equal_files).push_back(std::vector<size_t>({i}));
                            }
                            added = true;
                            (equal_files).back().push_back(j);
                            used_files[j] = true;
                        }
                        flag_eq = true;
                    }
                }
                for (std::vector<std::unique_ptr<std::ifstream>>::size_type i = 0; i < filesin.size(); i++) {
                    (*filesin[i]).close();
#ifdef DEBUG
                    std::cout << "close file -> " << mas_strings[i] << std::endl;
#endif
                }
                for (uint64_t h = eq_files_groups ; h < (equal_files).size(); h++){
                    for (std::vector<std::vector<QFileInfo>::size_type>::size_type i = 0; i < (equal_files)[h].size(); i++) {
                        mutex->lock();
                        (*do_out).push(std::make_pair(std::move(elem.second[(equal_files)[h][i]]), used_groups));
                        mutex->unlock();
                    }
                    used_groups = !used_groups;
                }
            }else{
                for (uint64_t h = 0 ; h < elem.second.size(); h++){
                    mutex->lock();
                    (*do_out).push(std::make_pair(std::move(elem.second[h]), used_groups));
                    mutex->unlock();
                }
                used_groups = !used_groups;
            }
        }
        (files_checked) += elem.second.size();
        emit that->status(files_checked);
        if (!that->is_scanning_now){
            return;
        }
    }
    *thread_end = true;
}


void FileDigger::do_file_search(){
    is_scanning_now = true;
    QDir dira = QDir(working_dir);
    std::map<int64_t, std::vector<QFileInfo>> same_size;

    int files_in_directory = 0;

    auto args = std::bind(&FileDigger::make_groups, this, dira, &same_size, &files_in_directory);
    auto handle = std::thread(args);
    bool thread_end = false;
    std::queue<std::pair<QFileInfo, bool>> out_queue;
    std::mutex mutex;
    handle.join();

    emit status_range(files_in_directory);

    auto args2 = std::bind(&FileDigger::add_to_ui,
                           this,
                           &thread_end,
                           &out_queue,
                           &mutex
                           );

    auto handle3 = std::thread(args2);

    FileDigger::check_files_eq(
                this,
                &same_size,
                &thread_end,
                &out_queue,
                &mutex
                );
    emit finished();
    handle3.join();
    is_scanning_now = false;
}

void FileDigger::stop_scanning()
{
    is_scanning_now = false;
}



void FileDigger::make_groups(FileDigger * that, QDir &dir, std::map<int64_t, std::vector<QFileInfo>> *array, int *files_in_directory) {
    QFileInfoList list = dir.entryInfoList();
    for (QFileInfo &file_info : list) {
#ifdef DEBUG
        std::cout << "Doing groups in -> " << file_info.absoluteFilePath().toStdString() << std::endl;
#endif
        if (!that->is_scanning_now){
            return;
        }
        if (file_info.fileName() == "." || file_info.fileName() == "..") {
            continue;
        }
        if (file_info.isSymLink()){
            continue;
        }
        if (file_info.isDir()) {
            dir.cd(file_info.fileName());
            make_groups(that, dir, array, files_in_directory);
            dir.cdUp();
        }
        if (file_info.isFile()) {
            ++(*files_in_directory);
            if (array->find(file_info.size()) != array->end()) {
                (*array)[file_info.size()].push_back(file_info);
            } else {
                std::vector<QFileInfo> temp;
                temp.push_back(file_info);
                (*array).insert(std::make_pair(file_info.size(), temp));
            }
        }
        if (!that->is_scanning_now){
            return;
        }
    }
}


void FileDigger::add_to_ui(FileDigger * that, bool *thread_end,
                           std::queue<std::pair<QFileInfo, bool>> *out_queue,
                           std::mutex *mutex){
    int files_added = 0;
    bool current_bool = true;
    QTreeWidgetItem *itemFather;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    while (!(*thread_end)){
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        // add group
        if ((*out_queue).size()){
            itemFather = new QTreeWidgetItem();
            while(true){
                if (!(*out_queue).empty() && (*out_queue).front().second == current_bool){
                    QTreeWidgetItem *item1 = new QTreeWidgetItem();
                    auto elem1 = (*out_queue).front().first;
                    ++files_added;
                    item1->setText(0, elem1.path());
                    item1->setText(1, elem1.fileName());
                    item1->setText(2, QString::number(elem1.size()));
                    itemFather->addChild(item1);
                    (*mutex).lock();
                    (*out_queue).pop();
                    (*mutex).unlock();
                }else if (thread_end){
                    break;
                } else if (!(*out_queue).empty()){
                    break;
                }else{
                    while ((*out_queue).empty() && !thread_end){
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                }
            }
            itemFather->setText(0,"Number of copies : " + QString::number(itemFather->childCount()));
            itemFather->setText(2,"Size : " + itemFather->child(0)->text(2) + "bytes.");
            emit that->ready_to_add(itemFather);
            current_bool = !current_bool;
        }
    }

    while ((*out_queue).size()){
        itemFather = new QTreeWidgetItem();
        while (!(*out_queue).empty() && (*out_queue).front().second == current_bool){
            QTreeWidgetItem *item1 = new QTreeWidgetItem();
            auto elem1 = (*out_queue).front().first;
            ++files_added;
            item1->setText(0, elem1.path());
            item1->setText(1, elem1.fileName());
            item1->setText(2, QString::number(elem1.size()));
            itemFather->addChild(item1);
            (*out_queue).pop();
        }
        itemFather->setText(0,"Number of copies : " + QString::number(itemFather->childCount()));
        itemFather->setText(2,"Size : " + itemFather->child(0)->text(2) + "bytes.");
        emit that->ready_to_add(itemFather);
        current_bool = !current_bool;
    }

}
