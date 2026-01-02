#ifndef DASHBOARDWINDOW_H
#define DASHBOARDWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQueryModel>

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
    void showBrowseView();
    void showStructureView();
    void showTablesView();
    void on_actionExit_triggered();
    void on_actionAbout_triggered();

private:
    void setQueryModel(const QString &title, const QString &queryText);

    Ui::DashboardWindow *ui;
    QSqlDatabase db;
    QSqlQueryModel *model;
};

#endif // DASHBOARDWINDOW_H
