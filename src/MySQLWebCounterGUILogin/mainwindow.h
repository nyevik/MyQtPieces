#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtSql>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <QDebug>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class DashboardWindow;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_loginButton_clicked();


private:
	Ui::MainWindow *ui;
	QSqlDatabase db;
	DashboardWindow *dashboardWindow;
};

#endif // MAINWINDOW_H
