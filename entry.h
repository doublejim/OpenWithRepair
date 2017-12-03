#ifndef ENTRY_H
#define ENTRY_H

#include <QString>

struct Entry{
    QString fileName;
    QString command;

    Entry() {}

    Entry( QString name, QString com)
    {
        fileName = name;
        command = com;
    }
};

#endif // ENTRY_H
