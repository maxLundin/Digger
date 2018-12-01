#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <QStatusBar>



class status_bar : public QStatusBar
{
    Q_OBJECT

public:

    explicit status_bar(QWidget *parent = nullptr);

};

#endif // STATUS_BAR_H
