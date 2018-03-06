#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPaintEvent>
#include "math.h"
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QProcess>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void startRequest(QUrl url);
    void paintEvent(QPaintEvent*);

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent*);
    //拦截最大化事件
    void changeEvent(QEvent *event);

private slots:
    void on_btnClose_clicked();
    void on_btnMin_clicked();
    void on_btnRun_clicked();

private slots:
    void downloadFile();
    void cancelDownload();
    void httpFinished();
    void httpReadyRead();
    void updateDataReadProgress(qint64 bytesRead, qint64 totalBytes);
    void enableDownloadButton(bool enabled);
    void slotAuthenticationRequired(QNetworkReply*,QAuthenticator *);
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply*,const QList<QSslError> &errors);
#endif
private:
    QString ConvertFromBase64(QString &input);
    QString ReadConfig();
    void PostData();

private:
    Ui::MainWindow *ui;

    QUrl url;
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
    QFile *file;
    int httpGetId;
    bool httpRequestAborted;
    QString fileName;
    qint64 haveread;

    QPoint dPos;
    bool isMousePress;

    bool isUpdated;

    QStringList AppArgs;
};

#endif // MAINWINDOW_H
