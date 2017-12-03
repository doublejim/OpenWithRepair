#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tableWidget->installEventFilter( &m_eventFilter);
    connect( &m_eventFilter, &EventFilterQt::eventActivated, this, &MainWindow::on_deleteEntry);

    m_regMan.gatherDataFromRegistry();

    m_skipUpdateWatcher = true;
    populateTable( m_regMan.m_resultsMapCR);
    ui->tableWidget->setHorizontalHeaderLabels({ "Application filename", "Command", "Problem description"});
    ui->tableWidget->horizontalHeader()->resizeSection( 1, 400);        // Command column size.

    updateInterface();

    ui->checkBoxUnknownExtensions->setChecked( m_regMan.webserviceToFindProgramIsEnabled());

    m_skipUpdateWatcher = false;

    if (m_regMan.m_resultsMapCR.isEmpty())
    {
        QMessageBox message;
        message.setWindowTitle("Error");
        message.setText("No entries were read from the registry. Check that you have permission.");
        message.exec();
    } else {
        ui->tableWidget->selectRow(0);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateTable( QMap<QString,Entry>& mapCR)
{
    ui->tableWidget->setRowCount( mapCR.size());
    ui->tableWidget->setColumnCount( 3);

    QMapIterator<QString,Entry> it (mapCR);

    int index = 0;

    while (it.hasNext())
    {
        it.next();
        QTableWidgetItem* appNameItem = new QTableWidgetItem( it.value().fileName);
        appNameItem->setData( Qt::UserRole, it.value().fileName);
        appNameItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        QTableWidgetItem* commandItem = new QTableWidgetItem( it.value().command);
        commandItem->setData( Qt::UserRole, it.value().command);
        resetHeaderItemColor( index);

        ui->tableWidget->setItem( index, 0, appNameItem);
        ui->tableWidget->setItem( index, 1, commandItem);
        applyCommandItemColorAndErrorMsg( commandItem);
        ++index;
    }
}

bool MainWindow::entryIsValid(QString& command)
{
    bool error = false;

    if (!pathExistsInCommand( command) || !commandContainsPercOneOrPercL( command))
        error = true;

    if (commandPathContainsSpace( command) && !commandPathHasQuotes( command))
        error = true;

    return !error;
}

void MainWindow::applyCommandItemColorAndErrorMsg(QTableWidgetItem* item)
{
    QString command = item->data(Qt::DisplayRole).toString();

    bool error = false;
    QString errorMsg;

    if (!pathExistsInCommand( command))
    {
        errorMsg = "Path doesn't exist or isn't executable.";
        error = true;
    }

    if (!commandContainsPercOneOrPercL( command))
    {
        errorMsg = "Command doesn't have %1.";
        error = true;
    }

    if (commandPathContainsSpace( command) && !commandPathHasQuotes( command))
    {
        errorMsg = "Path contains space but doesn't have quotes.";
        error = true;
    }

    if (error)
        item->setTextColor( m_badColor);
    else
        item->setTextColor( m_goodColor);

    // Set errorMessage (empty in case of no error):

    QTableWidgetItem* errorDescItem = new QTableWidgetItem(errorMsg);
    errorDescItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tableWidget->setItem( item->row(), 2, errorDescItem);
}

bool MainWindow::pathExistsInCommand(const QString& command)
{
    QString path;
    replacePathInCommand( command, path);
    QFile file(path);
    return file.exists();
}

bool MainWindow::commandPathHasQuotes(const QString& command)
{
    for (QString& fileType : m_allowedFileTypes)
    {
        QRegExp regex("\"?:*." + fileType + "\"");
        regex.setPatternSyntax(QRegExp::Wildcard);
        regex.setCaseSensitivity(Qt::CaseInsensitive);
        int pos = regex.indexIn( command);

        if (pos > -1)
            return true;
    }
    return false;
}

int MainWindow::replacePathInCommand( const QString& input, QString& output)
{
    for (QString& fileType : m_allowedFileTypes)
    {
        QRegExp regex("?:*." + fileType);                       // A file path.
        regex.setPatternSyntax(QRegExp::Wildcard);
        regex.setCaseSensitivity(Qt::CaseInsensitive);
        int pos = regex.indexIn( input);

        if (pos > -1)
        {
            output = regex.cap(0);
            return pos;
        } else {
            output = QString();
        }
    }
    return -1;
}

bool MainWindow::commandPathContainsSpace(QString &command)
{
    QString path;
    replacePathInCommand( command, path);
    return (path.indexOf(' ') > -1);
}

bool MainWindow::commandContainsPercOneOrPercL(const QString& command)     // %1 or %L
{
    QRegExp regex("(%1|%L)");
    regex.setCaseSensitivity(Qt::CaseInsensitive);
    int pos = regex.indexIn( command);

    if (pos > -1)
        return true;

    return false;
}

void MainWindow::applyChangedEntries()
{
    int previousSelectedRow = ui->tableWidget->currentRow();

    bool showErrorMsg = false;
    QStringList failedEntries;

    // Update each changed entry.

    for ( Entry& entry : m_changedEntries.values())
    {
        // qDebug() << "ENTRY:" << entry.command;

        bool error = m_regMan.setCommand( HKEY_CLASSES_ROOT, "Applications", entry.fileName, entry.command );

        if (error)
        {
            failedEntries.append( entry.fileName);
            showErrorMsg = true;
        }
    }

    for ( QString& name_of_entry : m_entriesMarkedForDeletion.keys())
    {
        Entry entry = m_regMan.m_resultsMapCR.value( name_of_entry.toUpper());
        bool error = m_regMan.removeEntry( HKEY_CLASSES_ROOT, "Applications", entry.fileName);

        if (error)
        {
            failedEntries.append( entry.fileName);
            showErrorMsg = true;
        }
    }

    m_changedEntries.clear();
    m_entriesMarkedForDeletion.clear();

    if (showErrorMsg)
    {
        QString failedListAsString;

        for (const QString& failed : failedEntries)
            failedListAsString.append( failed + ';');

        QMessageBox message;
        message.setWindowTitle("Error");
        message.setText("Failed at making changes to these registry entries: " + failedListAsString);
        message.exec();
    }

    // Recreate whole table.

    m_skipUpdateWatcher = true;
    m_regMan.gatherDataFromRegistry();
    populateTable( m_regMan.m_resultsMapCR);
    m_skipUpdateWatcher = false;

    if (ui->tableWidget->rowCount() > previousSelectedRow)
    {
        ui->tableWidget->selectRow( previousSelectedRow);
    } else {
        ui->tableWidget->selectRow( ui->tableWidget->rowCount() - 1);
    }
}

void MainWindow::resetHeaderItemColor( int row)
{
    // Reset header item to black.
    QTableWidgetItem* headerItem = ui->tableWidget->verticalHeaderItem( row);
    if (headerItem != nullptr)
        headerItem->setTextColor( Qt::black);
}

void MainWindow::abortChanges()
{
    m_skipUpdateWatcher = true;

    for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
    {
        // Change command value back to default.
        QTableWidgetItem* commandItem = ui->tableWidget->item( i, 1);
        QString defaultCommandData = commandItem->data( Qt::UserRole).toString();
        commandItem->setData( Qt::DisplayRole, defaultCommandData);

        resetHeaderItemColor(i);
        applyCommandItemColorAndErrorMsg( commandItem);
    }

    m_changedEntries.clear();
    m_entriesMarkedForDeletion.clear();
    updateInterface();

    m_skipUpdateWatcher = false;
}

void MainWindow::on_tableWidget_cellChanged(int currentRow, int currentColumn)
{
    // When the value of a cell has changed.

    if (m_skipUpdateWatcher)
        return;

    if (currentColumn != 1)        // Fx.: Don't continue here when changing color of items in column 0.
        return;

    // Store changed entry:

    QTableWidgetItem* commandItem = ui->tableWidget->item( currentRow, 1);

    QString changedCommand = commandItem->data(Qt::DisplayRole).toString();
    QTableWidgetItem* appNameItem = ui->tableWidget->item( currentRow, 0);
    QString appName = appNameItem->data(Qt::DisplayRole).toString();

    if (commandItem->data(Qt::UserRole).toString() == commandItem->data(Qt::DisplayRole).toString())
    {
        // Not changed from default. Reset header to black.

        QTableWidgetItem* item = ui->tableWidget->verticalHeaderItem( currentRow);
        item->setTextColor( Qt::black);

        removeFromListChange( appName);
    } else {

        // Changed from default. Change header to blue.

        QTableWidgetItem* item = new QTableWidgetItem( QString::number( currentRow + 1));
        item->setTextColor( Qt::darkMagenta);
        ui->tableWidget->setVerticalHeaderItem( currentRow, item);

        // extractPathFromSelection( currentRow);

        // Insert as a changed item.
        m_changedEntries.insert( appName, Entry( appName, changedCommand));
    }

    removeFromListDelete( appName);                                                 // We're changing, not deleting.
    applyCommandItemColorAndErrorMsg( commandItem);

    // Show proper status message.
    updateInterface();
}

void MainWindow::updateInterface()
{ 
    int numOfChanges = m_changedEntries.size() + m_entriesMarkedForDeletion.size();

    if (numOfChanges == 0)
    {
        ui->statusBar->showMessage( "Ready");
        ui->btnApply->setDisabled(true);
        ui->btnUndoChanges->setDisabled(true);
    } else {
        QString changeStr = numOfChanges == 1 ? "change" : "changes";
        ui->statusBar->showMessage( QString::number(numOfChanges) + " " + changeStr + " pending.");
        ui->btnApply->setEnabled(true);
        ui->btnUndoChanges->setEnabled(true);
    }
}

void MainWindow::on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{   // Cell selection has changed.

    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);
    Q_UNUSED(currentColumn);

    extractPathFromSelection( currentRow);
}

void MainWindow::extractPathFromSelection( int currentRow)     // Extract path. Split before command value into before and after the found path.
{
    QTableWidgetItem* commandItem = ui->tableWidget->item( currentRow, 1);
    QString commandValue = commandItem->data(Qt::DisplayRole).toString();
    QString extractedPath;
    int index = replacePathInCommand( commandValue, extractedPath);

    if (index > -1)
    {
        m_commandBeforePath = commandValue.mid( 0, index);
        m_commandAfterPath = commandValue.mid( index + extractedPath.size(), commandValue.size());
    } else {
        m_commandBeforePath = "\"";
        m_commandAfterPath = "\" \"%1\"";
    }

    ui->editPath->setText( extractedPath);
}

void MainWindow::insertFilePath( const QString& filePath)
{
    QTableWidgetItem* commandItem = ui->tableWidget->item( ui->tableWidget->currentRow(), 1);
    QString next_command = m_commandBeforePath + filePath + m_commandAfterPath;
    commandItem->setData( Qt::DisplayRole, next_command);
}

void MainWindow::on_editPath_textEdited(const QString &arg1)
{
    insertFilePath( arg1);
}

void MainWindow::removeFromListDelete( QString appName)
{
    m_entriesMarkedForDeletion.remove( appName);
}

void MainWindow::removeFromListChange( QString appName)
{
    m_changedEntries.remove( appName);
}

void MainWindow::on_deleteEntry(Events e)
{
    if (e != K_DELETE)
        return;

    QTableWidgetItem* appNameItem = ui->tableWidget->item( ui->tableWidget->currentRow(), 0);
    QString appName = appNameItem->data( Qt::DisplayRole).toString();
    QTableWidgetItem* commandItem = ui->tableWidget->item( ui->tableWidget->currentRow(), 1);

    int currentRow = ui->tableWidget->currentRow();

    if (m_entriesMarkedForDeletion.contains( appName))          // Un-mark for deletion.
    {
        // Reset color of item colored for deletion.
        QTableWidgetItem* item = ui->tableWidget->verticalHeaderItem( currentRow);
        item->setTextColor( Qt::black);

        removeFromListDelete( appName);

    } else {                                                    // Mark for deletion.

        // First, remove from list of items to change (we're deleting instead of changing).
        removeFromListChange( appName);

        commandItem->setData( Qt::DisplayRole, commandItem->data( Qt::UserRole));       // Reset command to default (since we're deleting, not changing).

        QTableWidgetItem* item = new QTableWidgetItem( QString::number( currentRow + 1));
        item->setTextColor( Qt::red);
        ui->tableWidget->setVerticalHeaderItem( currentRow, item);

        m_entriesMarkedForDeletion.insert( appName, true);                              // the bool isn't used for anything.
    }

    updateInterface();
}

void MainWindow::on_actionHelp_triggered()
{
    HelpDialog* help_dialog = new HelpDialog(this);
    help_dialog->show();
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog* about_dialog = new AboutDialog(this);
    about_dialog->show();
}

void MainWindow::on_btnUndoChanges_clicked()
{
    abortChanges();
}

void MainWindow::on_btnApply_clicked()
{
    applyChangedEntries();
    updateInterface();
}

void MainWindow::browseForDirectory()
{
    QFileDialog dialog;
    dialog.setFileMode( QFileDialog::ExistingFile);

    QString fileTypes;

    for (QString fileType : m_allowedFileTypes)
        fileTypes.append("*." + fileType + " ");

    fileTypes.chop(1);  // cut off last space.

    dialog.setNameFilter( "Applications (" + fileTypes + ")");
    dialog.setWindowTitle( "Select application path");

    QFile file = ui->editPath->text();
    QFileInfo finfo (file);
    QDir dir = finfo.absoluteDir();
    if (dir.exists())
    {
        dialog.setDirectory( dir);
        dialog.selectFile( finfo.fileName());
    }
    else
        dialog.setDirectory(QDir::currentPath());

    if (dialog.exec())
    {
        QFile chosenFile( dialog.selectedFiles()[0] );
        QFileInfo chosenFinfo (chosenFile);
        QDir chosenDir = chosenFinfo.absoluteDir();
        QString nativePath = QDir::toNativeSeparators( chosenDir.absolutePath()) + '\\' + chosenFinfo.fileName();
        ui->editPath->setText( nativePath );
        insertFilePath( nativePath );
    }
}

void MainWindow::on_pushButton_clicked()
{
    browseForDirectory();
}

void MainWindow::on_checkBoxUnknownExtensions_toggled(bool checked)
{
    if (m_skipUpdateWatcher)
        return;

    m_skipUpdateWatcher = true;

    if (!m_regMan.setWebServiceEnabled( checked))
    {
        ui->checkBoxUnknownExtensions->setChecked( !checked);

        QMessageBox message;
        message.setWindowTitle("Error");
        message.setText("Failed when writing to registry. Check that you have permission.");
        message.exec();
    }

    m_skipUpdateWatcher = false;
}
