#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dashboardwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	dashboardWindow(nullptr)
{
	ui->setupUi(this);

	db = QSqlDatabase::addDatabase("QMYSQL"); // Use MySQL database driver
	db.setHostName("127.0.0.1");
	db.setPort(3306);
    db.setDatabaseName("website_visitor_counter");
    qDebug() << db.databaseName();
    qDebug() << db.hostName();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_loginButton_clicked()
{
	QString username = ui->userInput->text().trimmed();
	QString password = ui->passwordInput->text();

	if (username.isEmpty())
	{
		QMessageBox::information(this, "Login", "Please enter a username.");
		return;
	}

	if (db.isOpen())
	{
		db.close();
	}

	db.setUserName(username);
	db.setPassword(password);

	if (!db.open())
	{
		QMessageBox::critical(this, "Login failed.", db.lastError().text());
		return;
	}

	QMessageBox::information(this, "Login success.", "You have successfully logged in!");
	if (!dashboardWindow)
	{
		dashboardWindow = new DashboardWindow(db);
		dashboardWindow->setAttribute(Qt::WA_DeleteOnClose);
		connect(dashboardWindow, &QObject::destroyed, this, [this]()
		{
			dashboardWindow = nullptr;
			this->show();
		});
	}
	dashboardWindow->show();
	dashboardWindow->raise();
	dashboardWindow->activateWindow();
	this->hide(); // Hide the login window when the dashboard is shown
}
