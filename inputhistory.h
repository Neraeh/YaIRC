#ifndef INPUTHISTORY_H
#define INPUTHISTORY_H

#include <QStringList>

class InputHistory : public QStringList
{
public:
    explicit InputHistory();
    void append(const QString &t);
    void resetCursor();
    QString previous();
    QString next();

private:
    int cursor;
};

#endif // INPUTHISTORY_H
