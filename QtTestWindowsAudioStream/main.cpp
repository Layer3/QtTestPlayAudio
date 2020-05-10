#include "QtTestWindowsAudioStream.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QtTestWindowsAudioStream w;
	w.show();
	return a.exec();
}
