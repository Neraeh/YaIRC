#include "inputhistory.h"

InputHistory::InputHistory() : QStringList()
{
    cursor = -1;
}

void InputHistory::append(const QString &t)
{
    QStringList::append(t);
    this->resetCursor();
}

void InputHistory::resetCursor()
{
    cursor = this->size();
}

QString InputHistory::previous()
{
    if (cursor - 1 > -1 && cursor <= this->size() && !this->isEmpty())
        return this->at(--cursor);
    else {
        cursor = -1;
        return QString();
    }
}

QString InputHistory::next()
{
    if (cursor < this->size() - 1 && cursor >= -1 && !this->isEmpty())
        return this->at(++cursor);
    else {
        cursor = this->size();
        return QString();
    }
}
