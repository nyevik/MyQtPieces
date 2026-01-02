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
    db.setUserName("bob");
    db.setPassword("");
    qDebug() << db.databaseName();
    qDebug() << db.hostName();

	if (db.open())
	{
		QMessageBox::information(this, "Database Connection", "Connected to the database successfully!");
		qDebug() << "Connected!";
	}
	else
	{
		QMessageBox::critical(this, "Database Connection", "Failed to connect to the database.");
		qDebug() << "Failed to connect.";

	}
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_loginButton_clicked()
{
	QString username = ui->userInput->text();
	QString password = ui->passwordInput->text();

	qDebug() << username << password;

	//QString command = "SELECT * FROM  WHERE username = '" + username + "' AND password = '" + password + "' AND status = 0";
	QString command = "SELECT * FROM pages";
    qDebug() << command;
	QSqlQuery query(db);
    //qDebug() << "query is " << quer
    if (query.exec(command))
	{
		if (query.size() > 0)
		{
			QMessageBox::information(this, "Login success.", "You have successfully logged in!");
			qDebug() << "Login successful!";
			while (query.next())
			{
				QString pageName = query.value("name").toString();
				int totalViews = query.value("total_views").toInt();
				qDebug() << "Page:" << pageName << "Total Views:" << totalViews;
			}	
			if (!dashboardWindow)
			{
				dashboardWindow = new DashboardWindow();
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
		else
		{
			QMessageBox::information(this, "Login failed.", "Login failed. Please try again...");
		}
	}
	else
	{
		QMessageBox::critical(this, "Query Error", "Failed to execute the query.");
		qDebug() << "Query execution failed: " << query.lastError().text();
	}
}

