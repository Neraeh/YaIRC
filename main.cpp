#include <QApplication>
#include <QTextCodec>
#include "yairc.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    YaIRC app;
    app.show();

    return a.exec();
}
