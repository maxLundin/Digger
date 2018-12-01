#include "treewidget.h"

#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <iostream>


tree_widget::tree_widget(QWidget *parent) : QTreeWidget (parent), size(0), expanded(false)
{
    setUniformRowHeights(1);
}

void tree_widget::open_everything(){
    std::cout << "expanding" << std::endl;
    if (expanded){
        collapseAll();
    }else{
        expandAll();
    }
    expanded = !expanded;
}

void tree_widget::add_to_tree(QTreeWidgetItem *data)
{
    if (data != nullptr){
        insertTopLevelItem(size++, data);
    }
}

void tree_widget::clear_tree(){
    clear();
    size = 0;
}

void tree_widget::delete_files(){
    int size1 = size;
    for (int i = 0 ; i < size1; ++i){
        auto * temp = takeTopLevelItem(i);
        size--;
        for (int j = 1 ; j < temp->childCount() ; j++){
            auto * child = temp->child(j);
            QFile file(getFileName(child));
            file.remove();
        }
        delete temp;
    }
}

QString tree_widget::getFileName(QTreeWidgetItem * selected_item)
{
#ifdef WINDOWS
    return selected_item->text(0)+ "\\" +selected_item->text(1);
#else
    return selected_item->text(0)+ "/" +selected_item->text(1);
#endif
}

void tree_widget::on_tree_item_clicked(QTreeWidgetItem *button){
#ifdef DEBUG
    std::cout << (button->text(0) + button->text(1)).toStdString() << std::endl;
#endif
    QString fileName = getFileName(button);
    QFileInfo dir(fileName);
    if (dir.isFile()){
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
    }
}
