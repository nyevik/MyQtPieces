/** 
 * @author Nik 
 */


#include "mainwindow.h"

// Support different Qt header layouts (Qt4/Qt5/Qt6) and varying include paths.
#if defined(__has_include)
#  if __has_include(<QApplication>)
#    include <QApplication>
#  elif __has_include(<QtWidgets/QApplication>)
#    include <QtWidgets/QApplication>
#  elif __has_include(<QtGui/QApplication>)
#    include <QtGui/QApplication>
#  else
#    error "Cannot find QApplication header. Ensure Qt headers are in your include path and the Widgets/Gui module is enabled."
#  endif
#else
#  include <QApplication>
#endif

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
