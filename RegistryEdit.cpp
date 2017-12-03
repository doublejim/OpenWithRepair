#include "RegistryEdit.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

RegistryEdit::RegistryEdit()
{}

bool RegistryEdit::setCommand( HKEY rootKey, QString topDirectory, QString applicationName, QString command)
{
    HKEY hKey;                          // Handle
    bool error = false;

    if (ERROR_SUCCESS == RegOpenKeyEx( rootKey, (LPCWSTR) topDirectory.utf16(), NULL, KEY_READ, &hKey))
    {
        QString subkey = applicationName + "\\shell\\open\\command";

        DWORD retCode = RegSetKeyValue( hKey, (LPCWSTR) subkey.utf16(), NULL, REG_SZ, (LPCWSTR) command.utf16(), sizeof(ushort) * command.size());

        if (retCode != ERROR_SUCCESS)
            error = true;
    }
    RegCloseKey(hKey);

    return error;
}

bool RegistryEdit::removeEntry( HKEY rootKey, QString topDirectory, QString applicationName)
{
    HKEY hKey;                              // Handle
    bool error = false;

    if (ERROR_SUCCESS == RegOpenKeyEx( rootKey, (LPCWSTR) topDirectory.utf16(), NULL, KEY_READ, &hKey))
    {
        QString subkey = applicationName;
        DWORD retCode = RegDeleteTree( hKey, (LPCWSTR) subkey.utf16());

        if (retCode != ERROR_SUCCESS)
            error = true;
    }
    RegCloseKey(hKey);

    return error;
}

bool RegistryEdit::webserviceToFindProgramIsEnabled()
{
    HKEY hKey;                          // Handle
    DWORD vData = 0;
    DWORD cbData = sizeof(DWORD);
    QString rootKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";

    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, (LPCWSTR) rootKey.utf16(), NULL, KEY_READ, &hKey))
    {
        QString valueName = "NoInternetOpenWith";
        RegGetValue( hKey, NULL, (LPCWSTR) valueName.utf16(), RRF_RT_DWORD, NULL, &vData, &cbData);
    }

    if (vData)
        return false;       // if value is 1, the web service is disabled.
    else
        return true;        // if value doesn't exist, or has value 0, the web service is enabled.
}

bool RegistryEdit::setWebServiceEnabled(bool isEnabled)
{
    HKEY hKey;                              // Handle
    bool okay = true;
    DWORD value = (DWORD) !isEnabled;       // if it's enabled, the value should be 0. If it's disabled, the value should be 1.

    QString rootKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies";

    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, (LPCWSTR) rootKey.utf16(), NULL, KEY_READ, &hKey))
    {
        QString keyName = "Explorer";
        QString valueName = "NoInternetOpenWith";
        DWORD retCode = RegSetKeyValue( hKey, (LPCWSTR) keyName.utf16(), (LPCWSTR) valueName.utf16(), REG_DWORD, &value, sizeof(DWORD));

        if (retCode != ERROR_SUCCESS)
            okay = false;
    }
    RegCloseKey(hKey);

    return okay;
}

void RegistryEdit::gatherDataFrom( HKEY rootKey, QString topDirectory, QMap<QString, Entry>& resultsMap)
{
    HKEY hKey;                         // Handle

    if (ERROR_SUCCESS == RegOpenKeyEx( rootKey, (LPCWSTR) topDirectory.utf16(), NULL, KEY_READ, &hKey))
    {
        TCHAR    achKey[MAX_KEY_LENGTH];    // buffer for subkey name
        DWORD    cbName;                    // size of name string
        DWORD    cSubKeys=0;                // number of subkeys
        DWORD i, retCode;

        // Get the number of subkeys:

        retCode = RegQueryInfoKey(hKey, NULL, NULL, NULL, &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        // If there are any subkeys, we open the subkey "shell/open/command/",
        // where we retrieve the default value.

        if (cSubKeys)
        {
            for (i = 0; i < cSubKeys; i++)
            {
                cbName = MAX_KEY_LENGTH;

                // Find the name of the key at index i:

                retCode = RegEnumKeyEx(hKey, i, achKey, &cbName, NULL, NULL, NULL, NULL);

                if (retCode == ERROR_SUCCESS)
                {
                    // Get key (default) value.

                    QString key = QString::fromWCharArray(achKey);
                    TCHAR vData[MAX_KEY_LENGTH];
                    DWORD cbData = MAX_KEY_LENGTH;
                    QString subkey = key + "\\shell\\open\\command";

                    retCode = RegGetValue( hKey, (LPCWSTR) subkey.utf16(), NULL, RRF_RT_ANY, NULL, &vData, &cbData);

                    if (retCode == ERROR_SUCCESS )
                    {
                        QString value = QString::fromWCharArray((wchar_t*) vData);
                        resultsMap.insert( key.toUpper(), Entry( key, value));
                    }
                }
            }
        }
    }
    RegCloseKey(hKey);
}

void RegistryEdit::gatherDataFromRegistry()
{
    m_resultsMapCR.clear();
    gatherDataFrom( HKEY_CLASSES_ROOT, "Applications", m_resultsMapCR);
}
