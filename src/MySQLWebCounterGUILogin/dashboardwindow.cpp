#include "dashboardwindow.h"
#include "ui_dashboardwindow.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlError>

DashboardWindow::DashboardWindow(const QSqlDatabase &database, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::DashboardWindow),
      db(database),
      model(new QSqlQueryModel(this))
{
    ui->setupUi(this);

    ui->tableView->setModel(model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->browseButton, &QPushButton::clicked, this, &DashboardWindow::showBrowseView);
    connect(ui->structureButton, &QPushButton::clicked, this, &DashboardWindow::showStructureView);
    connect(ui->tablesButton, &QPushButton::clicked, this, &DashboardWindow::showTablesView);

    showBrowseView();
}

DashboardWindow::~DashboardWindow()
{
    delete ui;
}

void DashboardWindow::setQueryModel(const QString &title, const QString &queryText)
{
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        statusBar()->showMessage("Database connection is not open.");
        return;
    }

    model->setQuery(queryText, db);
    if (model->lastError().isValid()) {
        QMessageBox::critical(this, "Query Error", model->lastError().text());
        statusBar()->showMessage("Query failed.");
        return;
    }

    statusBar()->showMessage(title);
}

void DashboardWindow::showBrowseView()
{
    setQueryModel(
        "Browse: pages",
        "SELECT id, name, total_views FROM pages ORDER BY total_views DESC");
}

void DashboardWindow::showStructureView()
{
    setQueryModel(
        "Structure: pages",
        "SHOW COLUMNS FROM pages");
}

void DashboardWindow::showTablesView()
{
    setQueryModel(
        "Tables",
        "SHOW TABLES");
}

void DashboardWindow::on_actionExit_triggered()
{
    close();
}

void DashboardWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, "About", "Database console placeholder view.");
}
