#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "RegistryEdit.h"
#include "entry.h"
#include "EventFilterQt.h"
#include "HelpDialog.h"
#include "aboutdialog.h"

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QTimer>
#include <QFileDialog>
#include <QDebug>

namespace Ui {
class MainWindow;
}

/* Todo:
 * - Menu
 * X nope. F5 = refresh list?
 * X Remove blue when the change has been saved.
 * X Allow deletion of an entry.
 * X fixed. Both delete and edit = problems...
 * X nope. liste til højre, over alle de ting som bliver ændret.
 * X yes. Allow undo.
 * X check for %1 and space in filename.
 * X sort alphabetically.
 * X nope. red/blue rounded corner change indicator icon.
 * - use windows to search checkbox.
 *
 */

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();    

private slots:
    void on_actionHelp_triggered();
    void on_actionAbout_triggered();
    void on_tableWidget_cellChanged(int currentRow, int currentColumn);
    void on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_editPath_textEdited(const QString &arg1);
    void on_deleteEntry(Events e);
    void on_btnUndoChanges_clicked();
    void on_btnApply_clicked();
    void on_pushButton_clicked();

    void on_checkBoxUnknownExtensions_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    RegistryEdit m_regMan;
    QMap<QString, Entry> m_changedEntries;
    QMap<QString, bool> m_entriesMarkedForDeletion;
    QStringList m_allowedFileTypes = {"exe", "bat", "cmd"};
    QColor m_goodColor = QColor(20,160,40);
    QColor m_badColor = QColor(255,0,0);
    bool m_skipUpdateWatcher = false;
    EventFilterQt m_eventFilter;
    QString m_commandBeforePath;
    QString m_commandAfterPath;

    void updateInterface();
    void populateTable(QMap<QString, Entry> &mapCR);
    void applyChangedEntries();
    void abortChanges();

    bool pathExistsInCommand(const QString &command);
    void applyCommandItemColorAndErrorMsg(QTableWidgetItem* item);
    bool commandContainsPercOneOrPercL(const QString &command);
    bool entryIsValid(QString &command);
    bool commandPathContainsSpace(QString& command);
    bool commandPathHasQuotes(const QString &command);
    void removeFromListDelete(QString appName);
    void removeFromListChange(QString appName);
    int replacePathInCommand(const QString &input, QString &output);
    void extractPathFromSelection(int currentRow);
    void browseForDirectory();
    void insertFilePath(const QString &filePath);
    void resetHeaderItemColor(int row);
};

#endif // MAINWINDOW_H
