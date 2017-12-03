#ifndef REGISTRYMANAGER_H
#define REGISTRYMANAGER_H

#include "qt_windows.h"
#include "entry.h"
#include <QList>
#include <QMap>
#include <QByteArray>
#include <QMessageBox>
#include <QDebug>

enum Location {CURRENT_USER, CLASSES_ROOT};

class RegistryEdit
{
public:
    QMap<QString, Entry> m_resultsMapCR;
    RegistryEdit();
    void gatherDataFromRegistry();
    bool setCommand(HKEY rootKey, QString topDirectory, QString applicationName, QString command);
    bool removeEntry(HKEY rootKey, QString topDirectory, QString applicationName);
    bool webserviceToFindProgramIsEnabled();
    bool setWebServiceEnabled( bool isEnabled);

private:
    void gatherDataFrom(HKEY rootKey, QString topDirectory, QMap<QString, Entry> &resultsMap);
};

#endif // REGISTRYMANAGER_H
