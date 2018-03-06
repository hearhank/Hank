#include "mainwindow.h"
#include <QApplication>
#include <QMutex>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include <QStringList>
#include <QMessageBox>
#include <QDesktopServices>

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

int main(int argc, char *argv[])
{

    qInstallMessageHandler(outputMessage);

    QApplication a(argc, argv);
    if(QApplication::arguments().length()<2){
        qDebug()<<"Arg length less than 2.";
        return -1;
    }

    MainWindow w;
    w.show();
    return a.exec();
}


void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();

    QString text;
    switch(type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;

    case QtWarningMsg:
        text = QString("Warning:");
        break;

    case QtCriticalMsg:
        text = QString("Critical:");
        break;

    case QtFatalMsg:
        text = QString("Fatal:");
        break;
    default:
        break;
    }

    QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
    QString current_date = QString("(%1)").arg(current_date_time);
    QString message = QString("%1 %2 %3 %4").arg(text).arg(context_info).arg(msg).arg(current_date);

    //qDebug()<<message;
    //QFile file(QString("log%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd")));

    QFile file("log.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QTextStream text_stream(&file);
    text_stream << message << "\r\n";
    file.flush();
    file.close();

    mutex.unlock();
}
