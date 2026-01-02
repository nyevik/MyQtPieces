#include "dashboardwindow.h"
#include "ui_dashboardwindow.h"

#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScreen>
#include <QSharedPointer>
#include <QSqlError>
#include <QThread>
#include <QVBoxLayout>

DashboardWindow::DashboardWindow(const QSqlDatabase &database, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::DashboardWindow),
      db(database),
      tableModel(new QSqlTableModel(this, db)),
      queryModel(new QSqlQueryModel(this))
{
    ui->setupUi(this);

    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked |
                                   QAbstractItemView::SelectedClicked |
                                   QAbstractItemView::EditKeyPressed);

    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested,
            this, &DashboardWindow::showTableContextMenu);

    connect(ui->pagesButton, &QPushButton::clicked, this, &DashboardWindow::showPagesTable);
    connect(ui->pageViewsButton, &QPushButton::clicked, this, &DashboardWindow::showPageViewsTable);
    connect(ui->tablesButton, &QPushButton::clicked, this, &DashboardWindow::showTablesList);
    connect(ui->describePagesButton, &QPushButton::clicked, this, &DashboardWindow::showPagesStructure);
    connect(ui->describePageViewsButton, &QPushButton::clicked, this, &DashboardWindow::showPageViewsStructure);
    connect(ui->pageViewsTotalsButton, &QPushButton::clicked, this, &DashboardWindow::showPageViewsTotals);

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
}

void DashboardWindow::showPagesTable()
{
    setTableModel("Pages", "pages");
}

void DashboardWindow::showPageViewsTable()
{
    setTableModel("Page Views", "page_views");
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
    const QRect screen = dialog.screen()
                             ? dialog.screen()->availableGeometry()
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
