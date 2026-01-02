#ifndef DASHBOARDWINDOW_H
#define DASHBOARDWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QStringList>
#include <QList>

namespace Ui {
class DashboardWindow;
}

class DashboardWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DashboardWindow(QWidget *parent = nullptr);
    ~DashboardWindow();

private slots:
    void showBrowseView();
    void showStructureView();
    void showTablesView();
    void on_actionExit_triggered();
    void on_actionAbout_triggered();

private:
    void setPlaceholderModel(const QString &title,
                             const QStringList &columns,
                             const QList<QStringList> &rows);

    Ui::DashboardWindow *ui;
    QStandardItemModel *model;
};

#endif // DASHBOARDWINDOW_H
