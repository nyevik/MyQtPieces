#include "dashboardwindow.h"
#include "ui_dashboardwindow.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardItem>

DashboardWindow::DashboardWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::DashboardWindow),
      model(new QStandardItemModel(this))
{
    ui->setupUi(this);

    ui->tableView->setModel(model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->browseButton, &QPushButton::clicked, this, &DashboardWindow::showBrowseView);
    connect(ui->structureButton, &QPushButton::clicked, this, &DashboardWindow::showStructureView);
    connect(ui->tablesButton, &QPushButton::clicked, this, &DashboardWindow::showTablesView);

    showBrowseView();
}

DashboardWindow::~DashboardWindow()
{
    delete ui;
}

void DashboardWindow::setPlaceholderModel(const QString &title,
                                          const QStringList &columns,
                                          const QList<QStringList> &rows)
{
    model->clear();
    model->setHorizontalHeaderLabels(columns);

    for (const auto &row : rows) {
        QList<QStandardItem *> items;
        items.reserve(row.size());
        for (const auto &cell : row) {
            items.append(new QStandardItem(cell));
        }
        model->appendRow(items);
    }

    statusBar()->showMessage(title);
}

void DashboardWindow::showBrowseView()
{
    setPlaceholderModel(
        "Browse view (placeholder)",
        {"Table", "Rows", "Updated"},
        {
            {"users", "3", "today"},
            {"visits", "128", "today"},
            {"events", "12", "today"}
        });
}

void DashboardWindow::showStructureView()
{
    setPlaceholderModel(
        "Structure view (placeholder)",
        {"Column", "Type", "Nullable"},
        {
            {"id", "INT", "no"},
            {"username", "VARCHAR(255)", "no"},
            {"created_at", "DATETIME", "yes"}
        });
}

void DashboardWindow::showTablesView()
{
    setPlaceholderModel(
        "Tables view (placeholder)",
        {"Schema", "Table", "Engine"},
        {
            {"public", "users", "InnoDB"},
            {"public", "visits", "InnoDB"},
            {"public", "events", "InnoDB"}
        });
}

void DashboardWindow::on_actionExit_triggered()
{
    close();
}

void DashboardWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, "About", "Database console placeholder view.");
}
