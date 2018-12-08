#include "filedigger.h"

#include <bits/unique_ptr.h>
#include <QDir>
#include <QTreeWidget>
#include <fstream>
#include <thread>


FileDigger::FileDigger(QString dir) {
    working_dir = dir;
    is_scanning_now = false;
}

void FileDigger::check_files_eq(FileDigger *that,
                                std::map<int64_t, std::vector<QFileInfo>> *array,
                                bool *thread_end,
                                std::vector<std::pair<QFileInfo, bool> > *do_out,
                                std::mutex *mutex) {
    std::vector<std::vector<size_t>> equal_files;
    uint64_t eq_files_groups = 0;
    bool used_groups = true;
    int files_checked = 0;
    char buffer1[buffer_mas_length];
    char buffer2[buffer_mas_length];
    for (auto i = (*array).rbegin(); i != (*array).rend(); ++i) {
        auto &elem = (*i);
#ifdef DEBUG
        std::cout << "doing new group" << std::endl;
#endif
        if (!that->is_scanning_now) {
            *thread_end = true;
            return;
        }
        eq_files_groups = (equal_files).size();

        if (elem.second.size() > 1) {
            if (elem.first != 0) {
                std::vector<std::unique_ptr<std::ifstream>> filesin;
                filesin.reserve(elem.second.size());
#ifdef DEBUG
                std::vector<std::string> mas_strings;
#endif

                for (size_t i = 0; i < elem.second.size(); i++) {
#ifdef DEBUG
                    mas_strings.push_back(elem.second[i].absoluteFilePath().toStdString());
                    std::cout << "open file -> " << mas_strings.back() << std::endl;
#endif
                    std::unique_ptr<std::ifstream> temp;
                    temp.reset(new std::ifstream(elem.second[i].absoluteFilePath().toStdString(), std::ios::binary));
                    if (temp->is_open()) {
                        (filesin).push_back(std::move(temp));
                    } else {
                        std::cout << "Error opening file -> " <<
                                     elem.second[i].absoluteFilePath().toStdString() << std::endl;
                        for (int _ = 0; _ < 3; _++) {
                            std::cout << "Trying again -> " <<
                                         elem.second[i].absoluteFilePath().toStdString() << std::endl;
                            temp.reset(new std::ifstream(elem.second[i].absoluteFilePath().toStdString(),
                                                         std::ios::binary));
                            if (temp->is_open()) {
                                (filesin).push_back(std::move(temp));
                                std::cout << "It opened!!!" << std::endl;
                                break;
                            } else {
                                std::cout << "Error" << std::endl;
                            }
                        }
                    }

                }
                std::vector<bool> used_files(elem.second.size());

                bool flag_eq = false;
                for (size_t i = 0; i < filesin.size(); i++) {
                    if (used_files[i]) {
                        continue;
                    }
                    used_files[i] = true;
                    bool added = false;
                    flag_eq = true;
                    for (size_t j = i + 1; j < (filesin).size(); j++) {
                        if (used_files[j]) {
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
                            flag_eq = (memcmp(buffer1,buffer2,buff_length) == 0);
                            //                            for (std::streamsize k = 0; flag_eq && k < buff_length; k++) {
                            //                                flag_eq = (buffer1[k] == buffer2[k]);
                            //                            }
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
                for (uint64_t h = eq_files_groups; h < (equal_files).size(); h++) {
                    for (size_t i = 0; i < (equal_files)[h].size(); i++) {
                        mutex->lock();
                        (*do_out).push_back(std::make_pair(std::move(elem.second[(equal_files)[h][i]]), used_groups));
                        mutex->unlock();
                    }
                    used_groups = !used_groups;
                }
            } else {
                for (uint64_t h = 0; h < elem.second.size(); h++) {
                    mutex->lock();
                    (*do_out).push_back(std::make_pair(std::move(elem.second[h]), used_groups));
                    mutex->unlock();
                }
                used_groups = !used_groups;
            }
        }
        (files_checked) += elem.second.size();
        emit that->status(files_checked);
        if (!that->is_scanning_now) {
            *thread_end = true;
            return;
        }
    }
    *thread_end = true;
}


void FileDigger::do_file_search() {
    is_scanning_now = true;
    QDir dira = QDir(working_dir);
    std::map<int64_t, std::vector<QFileInfo>> same_size;

    int files_in_directory = 0;

    auto args = std::bind(&FileDigger::make_groups, this, dira, &same_size, &files_in_directory);
    auto handle = std::thread(args);
    bool thread_end = false;
    std::vector<std::pair<QFileInfo, bool>> out_queue;
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

void FileDigger::stop_scanning() {
    is_scanning_now = false;
}


void FileDigger::make_groups(FileDigger *that, QDir &dir, std::map<int64_t, std::vector<QFileInfo>> *array,
                             int *files_in_directory) {
    QFileInfoList list = dir.entryInfoList();
    for (QFileInfo &file_info : list) {
#ifdef DEBUG
        std::cout << "Doing groups in -> " << file_info.absoluteFilePath().toStdString() << std::endl;
#endif
        if (!that->is_scanning_now) {
            return;
        }
        if (file_info.fileName() == "." || file_info.fileName() == "..") {
            continue;
        }
        if (file_info.isSymLink()) {
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
        if (!that->is_scanning_now) {
            return;
        }
    }
}


void FileDigger::add_to_ui(FileDigger *that, bool *thread_end,
                           std::vector<std::pair<QFileInfo, bool>> *out_queue,
                           std::mutex *mutex) {
    int files_added = 0;
    bool current_bool = true;
    bool finished = true;
    std::unique_ptr<QTreeWidgetItem> itemFather;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    while (!(*thread_end)) {
        // add group


        mutex->lock();
        if ((*out_queue).size()) {
            std::vector<std::pair<QFileInfo, bool>> my_queue;
            my_queue.reserve(((*out_queue).size()));
            (*out_queue).swap(my_queue);
            mutex->unlock();
            size_t pos = 0;
            if (finished) {
                itemFather.reset(new QTreeWidgetItem());
            }
            while (pos != my_queue.size()) {
                while (pos != my_queue.size() && my_queue[pos].second == current_bool) {
                    QTreeWidgetItem *item1 = new QTreeWidgetItem();
                    ++files_added;
                    item1->setText(0, my_queue[pos].first.path());
                    item1->setText(1, my_queue[pos].first.fileName());
                    item1->setText(2, QString::number(my_queue[pos].first.size()));
                    itemFather->addChild(item1);
                    ++pos;
                }
                if (pos != my_queue.size()) {
                    finished = true;
                    if (itemFather->childCount() == 0) {
                        std::cout << "Something is wrong" << std::endl;
                    } else {
                        itemFather->setText(0, "Number of copies : " + QString::number(itemFather->childCount()));
                        itemFather->setText(2, "Size : " + itemFather->child(0)->text(2) + "bytes.");
                        emit that->ready_to_add(itemFather.get());
                        itemFather.release();
                        itemFather.reset(new QTreeWidgetItem());
                        current_bool = !current_bool;
                    }
                } else {
                    finished = false;
                }
            }


        } else {
            mutex->unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }


    size_t pos = 0;

    if (finished) {
        itemFather.reset(new QTreeWidgetItem());
    }
    while (pos != (*out_queue).size()) {
        while (pos != (*out_queue).size() && (*out_queue)[pos].second == current_bool) {
            QTreeWidgetItem *item1 = new QTreeWidgetItem();
            ++files_added;
            item1->setText(0, (*out_queue)[pos].first.path());
            item1->setText(1, (*out_queue)[pos].first.fileName());
            item1->setText(2, QString::number((*out_queue)[pos].first.size()));
            itemFather->addChild(item1);
            ++pos;
        }
        finished = true;
        itemFather->setText(0, "Number of copies : " + QString::number(itemFather->childCount()));
        itemFather->setText(2, "Size : " + itemFather->child(0)->text(2) + "bytes.");
        emit that->ready_to_add(itemFather.get());
        itemFather.release();
        itemFather.reset(new QTreeWidgetItem());
        current_bool = !current_bool;
    }

}
