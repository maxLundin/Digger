#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include <QTreeWidget>

class tree_widget : public QTreeWidget {
    Q_OBJECT

public:
    explicit tree_widget(QWidget *parent = nullptr);

    void setup_tree(QTreeWidget *);

    void add_to_tree(QTreeWidgetItem *);

    QString static getFileName(QTreeWidgetItem *);

signals:

public slots:
    void on_tree_item_clicked(QTreeWidgetItem *);

    void open_everything();

private:
    int size;
    bool expanded;
};

#endif // TREEWIDGET_H
