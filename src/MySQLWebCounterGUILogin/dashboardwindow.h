#ifndef DASHBOARDWINDOW_H
#define DASHBOARDWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QString>

namespace Ui {
class DashboardWindow;
}

class DashboardWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DashboardWindow(const QSqlDatabase &database, QWidget *parent = nullptr);
    ~DashboardWindow();

private slots:
    void showPagesTable();
    void showPageViewsTable();
    void showTablesList();
    void showPagesStructure();
    void showPageViewsStructure();
    void showPageViewsTotals();
    void showTableContextMenu(const QPoint &position);
    void onDigFinished(const QString &ip, const QString &output, const QString &error);
    void on_actionExit_triggered();
    void on_actionAbout_triggered();

private:
    void setTableModel(const QString &title, const QString &tableName);
    void setQueryModel(const QString &title, const QString &queryText);
    void startDigLookup(const QString &ip);

    Ui::DashboardWindow *ui;
    QSqlDatabase db;
    QSqlTableModel *tableModel;
    QSqlQueryModel *queryModel;
};

#endif // DASHBOARDWINDOW_H
