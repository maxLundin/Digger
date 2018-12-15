#ifndef FILEDIGGER_H
#define FILEDIGGER_H

#include <iostream>
#include <QFileInfo>
#include <map>
#include <vector>
#include <QTreeWidget>
#include <thread>

#include <bits/std_mutex.h>
#include <queue>

const size_t buffer_mas_length = 128000;

class FileDigger : public QObject
{
    Q_OBJECT

public:
    FileDigger(QString dir);


    void static check_files_eq(FileDigger * ,
                               std::map<int64_t, std::vector<QFileInfo> > *,
                               bool *,
                               std::vector<std::pair<QFileInfo, bool> > *,
                               std::mutex *);

    void static make_groups(FileDigger * , QDir &, std::map<int64_t, std::vector<QFileInfo>> *, int *);

    void static add_to_ui(FileDigger *,
                          bool *,
                          std::vector<std::pair<QFileInfo, bool>> *,
                          std::mutex *);

    bool isStopped();

    virtual ~FileDigger() = default;


public slots:
    void do_file_search();
    void stop_scanning();
signals:
    void ready_to_add(QTreeWidgetItem *);
    void status(int);
    void status_range(int);
    void finished();

private:
    QString working_dir;
    std::atomic<bool> is_scanning_now;
    bool stopped;
};

#endif // FILEDIGGER_H
