#include "dashboardwindow.h"
#include "ui_dashboardwindow.h"

#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScreen>
#include <QScrollBar>
#include <QSharedPointer>
#include <QSqlRecord>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardItem>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

DashboardWindow::DashboardWindow(const QSqlDatabase &database, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::DashboardWindow),
      db(database),
      tableModel(new QSqlTableModel(this, db)),
      queryModel(new QSqlQueryModel(this)),
      sortedModel(new QStandardItemModel(this)),
      sortThread(nullptr)
{
    ui->setupUi(this);

    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->horizontalHeader()->setStretchLastSection(false);
    ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked |
                                   QAbstractItemView::SelectedClicked |
                                   QAbstractItemView::EditKeyPressed);

    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested,
            this, &DashboardWindow::showTableContextMenu);

    connect(ui->pagesButton, &QPushButton::clicked, this, &DashboardWindow::showPagesTable);
    connect(ui->pageViewsButton, &QPushButton::clicked, this, &DashboardWindow::showPageViewsTable);
    connect(ui->sortPageIdButton, &QPushButton::clicked, this, &DashboardWindow::sortPageViewsById);
    connect(ui->sortPageNameButton, &QPushButton::clicked, this, &DashboardWindow::sortPageViewsByName);
    connect(ui->tablesButton, &QPushButton::clicked, this, &DashboardWindow::showTablesList);
    connect(ui->describePagesButton, &QPushButton::clicked, this, &DashboardWindow::showPagesStructure);
    connect(ui->describePageViewsButton, &QPushButton::clicked, this, &DashboardWindow::showPageViewsStructure);
    connect(ui->pageViewsTotalsButton, &QPushButton::clicked, this, &DashboardWindow::showPageViewsTotals);
    connect(ui->resetViewsButton, &QPushButton::clicked, this, &DashboardWindow::resetPageViewsTable);

    showPagesTable();
}

DashboardWindow::~DashboardWindow()
{
    delete ui;
}

void DashboardWindow::setTableModel(const QString &title, const QString &tableName)
{
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        statusBar()->showMessage("Database connection is not open.");
        return;
    }

    tableModel->setTable(tableName);
    tableModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    if (!tableModel->select()) {
        QMessageBox::critical(this, "Query Error", tableModel->lastError().text());
        statusBar()->showMessage("Query failed.");
        return;
    }

    ui->tableView->setModel(tableModel);
    ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked |
                                   QAbstractItemView::SelectedClicked |
                                   QAbstractItemView::EditKeyPressed);
    statusBar()->showMessage(title);
    scheduleResizeToContents();
}

void DashboardWindow::setQueryModel(const QString &title, const QString &queryText)
{
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        statusBar()->showMessage("Database connection is not open.");
        return;
    }

    queryModel->setQuery(queryText, db);
    if (queryModel->lastError().isValid()) {
        QMessageBox::critical(this, "Query Error", queryModel->lastError().text());
        statusBar()->showMessage("Query failed.");
        return;
    }

    ui->tableView->setModel(queryModel);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    statusBar()->showMessage(title);
    scheduleResizeToContents();
}

void DashboardWindow::showPagesTable()
{
    setTableModel("Pages", "pages");
}

void DashboardWindow::showPageViewsTable()
{
    setTableModel("Page Views", "page_views");
}

void DashboardWindow::sortPageViewsById()
{
    runPageViewsSort("page_id", "Page Views sorted by Page ID");
}

void DashboardWindow::sortPageViewsByName()
{
    runPageViewsSort("page_name", "Page Views sorted by Page Name");
}

void DashboardWindow::showTablesList()
{
    setQueryModel("Tables", "SHOW TABLES");
}

void DashboardWindow::showPagesStructure()
{
    setQueryModel("Describe pages", "SHOW COLUMNS FROM pages");
}

void DashboardWindow::showPageViewsStructure()
{
    setQueryModel("Describe page_views", "SHOW COLUMNS FROM page_views");
}

void DashboardWindow::showPageViewsTotals()
{
    setQueryModel(
        "Page Views + Totals",
        "SELECT pv.num, pv.visitor_ip, pv.page_id, p.name AS page_name, "
        "p.total_views, pv.tstamp "
        "FROM page_views pv "
        "JOIN pages p ON p.id = pv.page_id "
        "ORDER BY pv.tstamp DESC");
}

void DashboardWindow::runPageViewsSort(const QString &orderByColumn, const QString &title)
{
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        statusBar()->showMessage("Database connection is not open.");
        return;
    }
    if (sortThread && sortThread->isRunning()) {
        statusBar()->showMessage("Sort already in progress.");
        return;
    }

    const QString orderBy = (orderByColumn == "page_id") ? "page_id" : "page_name";
    const QString queryText =
        QString("SELECT num, visitor_ip, page_id, page_name, tstamp "
                "FROM page_views ORDER BY %1").arg(orderBy);

    const QString driver = db.driverName();
    const QString host = db.hostName();
    const QString dbName = db.databaseName();
    const QString user = db.userName();
    const QString password = db.password();
    const int port = db.port();
    const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    auto headers = QSharedPointer<QStringList>::create();
    auto rows = QSharedPointer<QVector<QStringList>>::create();
    auto error = QSharedPointer<QString>::create();

    ui->sortPageIdButton->setEnabled(false);
    ui->sortPageNameButton->setEnabled(false);
    statusBar()->showMessage(QString("Sorting page_views by %1...").arg(orderBy));

    sortThread = QThread::create([=]() {
        QString localError;
        {
            QSqlDatabase threadDb = QSqlDatabase::addDatabase(driver, connectionName);
            threadDb.setHostName(host);
            threadDb.setPort(port);
            threadDb.setDatabaseName(dbName);
            threadDb.setUserName(user);
            threadDb.setPassword(password);

            if (!threadDb.open()) {
                localError = threadDb.lastError().text();
            } else {
                QSqlQuery query(threadDb);
                if (!query.exec(queryText)) {
                    localError = query.lastError().text();
                } else {
                    const QSqlRecord record = query.record();
                    headers->reserve(record.count());
                    for (int i = 0; i < record.count(); ++i) {
                        headers->append(record.fieldName(i));
                    }

                    while (query.next()) {
                        QStringList row;
                        row.reserve(record.count());
                        for (int i = 0; i < record.count(); ++i) {
                            row.append(query.value(i).toString());
                        }
                        rows->append(row);
                    }
                }
                threadDb.close();
            }
        }
        QSqlDatabase::removeDatabase(connectionName);
        if (!localError.isEmpty()) {
            *error = localError;
        }
    });

    connect(sortThread, &QThread::finished, this, [this, title, headers, rows, error]() {
        ui->sortPageIdButton->setEnabled(true);
        ui->sortPageNameButton->setEnabled(true);
        sortThread->deleteLater();
        sortThread = nullptr;

        if (!error->isEmpty()) {
            QMessageBox::critical(this, "Query Error", *error);
            statusBar()->showMessage("Sort failed.");
            return;
        }

        applySortedResults(title, *headers, *rows);
    });

    sortThread->start();
}

void DashboardWindow::applySortedResults(const QString &title,
                                         const QStringList &headers,
                                         const QVector<QStringList> &rows)
{
    if (!sortedModel) {
        sortedModel = new QStandardItemModel(this);
    }

    sortedModel->clear();
    sortedModel->setHorizontalHeaderLabels(headers);
    for (const auto &row : rows) {
        QList<QStandardItem *> items;
        items.reserve(row.size());
        for (const auto &cell : row) {
            items.append(new QStandardItem(cell));
        }
        sortedModel->appendRow(items);
    }

    ui->tableView->setModel(sortedModel);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    statusBar()->showMessage(title);
    scheduleResizeToContents();
}

void DashboardWindow::scheduleResizeToContents()
{
    QTimer::singleShot(0, this, &DashboardWindow::resizeToContents);
}

void DashboardWindow::resizeToContents()
{
    auto *model = ui->tableView->model();
    if (!model) {
        return;
    }

    while (model->canFetchMore(QModelIndex())) {
        model->fetchMore(QModelIndex());
    }

    ui->tableView->resizeColumnsToContents();

    int columnsWidth = 0;
    for (int column = 0; column < model->columnCount(); ++column) {
        columnsWidth += ui->tableView->columnWidth(column);
    }

    int extraWidth = width() - ui->tableView->viewport()->width();
    if (extraWidth < 0) {
        extraWidth = 0;
    }
    if (ui->tableView->verticalScrollBar()->isVisible()) {
        extraWidth += ui->tableView->verticalScrollBar()->width();
    }

    int targetWidth = columnsWidth + extraWidth + 24;
    const QRect screenGeometry = QGuiApplication::primaryScreen()
                                     ? QGuiApplication::primaryScreen()->availableGeometry()
                                     : QRect(0, 0, 900, 600);
    const int maxWidth = screenGeometry.width() * 9 / 10;
    targetWidth = qBound(520, targetWidth, maxWidth);

    resize(targetWidth, height());
}

/**
 * @brief Resets the page_views table by deleting all records, 
 * resetting the auto-increment value, and updating the total_views in the pages table. 
 * 
 * @details This function performs the following steps:
 * 1. Deletes all records from the page_views table.
 * 2. Resets the auto-increment value of the page_views table to 1.
 * 3. Updates the total_views column in the pages table to 0.
 * @pre The database connection must be open.
 * @post The page_views table is empty, its auto-increment value is reset, and the total_views in the pages table is set to 0 for each page.
 */
void DashboardWindow::resetPageViewsTable()
{
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        statusBar()->showMessage("Database connection is not open.");
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("DELETE FROM page_views")) {
        QMessageBox::critical(this, "Query Error", query.lastError().text());
        statusBar()->showMessage("Failed to reset page_views table.");
        return;
    }

    if (!query.exec("ALTER TABLE page_views AUTO_INCREMENT = 1")) {
        QMessageBox::critical(this, "Query Error", query.lastError().text());
        statusBar()->showMessage("Failed to reset page_views table auto-increment.");
        return;
    }

    if (!query.exec("UPDATE pages SET total_views = 0")) {
        QMessageBox::critical(this, "Query Error", query.lastError().text());
        statusBar()->showMessage("Failed to reset total_views in pages table.");
        return;
    }

    statusBar()->showMessage("Page Views table has been reset.");
    showPageViewsTable();
}

void DashboardWindow::showTableContextMenu(const QPoint &position)
{
    if (ui->tableView->model() != tableModel) {
        return;
    }
    if (tableModel->tableName() != "page_views") {
        return;
    }

    QModelIndex index = ui->tableView->indexAt(position);
    if (!index.isValid()) {
        return;
    }

    int ipColumn = tableModel->fieldIndex("visitor_ip");
    if (ipColumn < 0 || index.column() != ipColumn) {
        return;
    }

    QString ip = tableModel->data(tableModel->index(index.row(), ipColumn)).toString().trimmed();
    if (ip.isEmpty()) {
        return;
    }

    QMenu menu(this);
    QAction *resolveAction = menu.addAction("Resolve IP (dig)");
    QAction *chosen = menu.exec(ui->tableView->viewport()->mapToGlobal(position));
    if (chosen == resolveAction) {
        startDigLookup(ip);
    }
}

void DashboardWindow::startDigLookup(const QString &ip)
{
    auto output = QSharedPointer<QString>::create();
    auto error = QSharedPointer<QString>::create();

    QThread *thread = QThread::create([ip, output, error]() {
        QProcess process;
        //process.start("dig", {"+short", "+time=2", "+tries=1", "-x", ip});
        process.start("dig", QStringList() << "-x" << ip);
        if (!process.waitForFinished(7000)) {//wait 7 seconds
            process.kill();
            process.waitForFinished();
            *error = "dig timed out.";
            return;
        }
        *output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        *error = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (output->isEmpty() && error->isEmpty()) {
            *error = "No DNS records returned.";
        }
    });

    connect(thread, &QThread::finished, this, [this, ip, output, error, thread]() {
        thread->deleteLater();
        onDigFinished(ip, *output, *error);
    });

    statusBar()->showMessage(QString("Resolving %1...").arg(ip));
    thread->start();
}

void DashboardWindow::onDigFinished(const QString &ip, const QString &output, const QString &error)
{
    QDialog dialog(this);
    dialog.setWindowTitle("DNS Lookup");
    dialog.setSizeGripEnabled(true);
    dialog.setModal(true);

    auto *layout = new QVBoxLayout(&dialog);
    auto *commandLabel = new QLabel(QString("dig -x %1").arg(ip), &dialog);
    commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    auto *textEdit = new QPlainTextEdit(&dialog);
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    QStringList parts;
    if (!output.isEmpty()) {
        parts << output;
    }
    if (!error.isEmpty()) {
        parts << QString("stderr:\n%1").arg(error);
    }
    if (parts.isEmpty()) {
        parts << "No DNS records returned.";
    }
    const QString fullText = parts.join("\n\n");
    textEdit->setPlainText(fullText);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    layout->addWidget(commandLabel);
    layout->addWidget(textEdit, 1);
    layout->addWidget(buttonBox);

    const QStringList lines = fullText.split('\n');
    const QFontMetrics metrics(textEdit->font());
    int maxWidth = 0;
    for (const auto &line : lines) {
        maxWidth = qMax(maxWidth, metrics.horizontalAdvance(line));
    }
    const int textHeight = qMax(1, lines.size()) * metrics.lineSpacing();
    const QRect screen = QGuiApplication::primaryScreen()
                             ? QGuiApplication::primaryScreen()->availableGeometry()
                             : QRect(0, 0, 900, 600);
    const int desiredWidth = qMin(screen.width() * 9 / 10, maxWidth + 120);
    const int desiredHeight = qMin(screen.height() * 8 / 10, textHeight + 140);
    dialog.resize(qMax(420, desiredWidth), qMax(240, desiredHeight));

    dialog.exec();
    statusBar()->showMessage(QString("Resolved %1").arg(ip), 5000);
}

void DashboardWindow::on_actionExit_triggered()
{
    close();
}

void DashboardWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, "About", "Database console placeholder view.");
}
