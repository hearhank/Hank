#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QtMath>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QAuthenticator>
#include "JlCompress.h"
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    QPainterPath path;
    //QRectF rect = QRectF(10,10,500,300);  //两点确定矩形范围，一般为控件大小，这样可以切割四个圆角，也可以调整大小，调整圆角个数
    path.addRoundedRect(10,10,500,300,3,3);   //后面两个参数的范围0-99，值越大越园
    QPolygon polygon= path.toFillPolygon().toPolygon();//获得这个路径上的所有的点
    QRegion region(polygon);//根据这些点构造这个区域
    ui->centralWidget->setMask(region);

    connect(&qnam, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            this, SLOT(slotAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
    haveread=0;

    ui->lblResultTip->hide();
    ui->progUpdate->hide();
    ui->progUpdate->setValue(0);
    QString title=QString("");
    ui->lblCaption->setText(title);
    ui->lblTitle->setText(title);

    QStringList sourceArgs = QApplication::arguments();
    QString sourceStr = this->ConvertFromBase64(sourceArgs[1]);

    this->AppArgs = sourceStr.split("|");
    if(AppArgs.count()<5)
    {
        qDebug()<<"App args length less than 5.";
        this->close();
        QApplication::exec();
    }

    isUpdated=false;
    //todo
    //    qDebug()<<"参数：";
    //    QStringList args = QApplication::arguments();
    //    for(int i=0;i<args.length();i++)
    //    {
    //        qWarning(QApplication::arguments()[i].toLatin1().data());
    //    }

    //QMessageBox::information(this,"提示","请先关闭爱店家，否则更新可能失败！",QMessageBox::Ok);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *event)
{
    if(event->type()!=QEvent::WindowStateChange)
        return;
    if(this->windowState()==Qt::WindowMaximized)
    {
        this->showNormal();
    }
}

void MainWindow::paintEvent(QPaintEvent *){
    int shadowLen=10;
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.addRoundedRect(shadowLen, shadowLen, this->width()-shadowLen*2, this->height()-shadowLen*2,3,3);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, QBrush(Qt::white));

    QColor color(20, 20, 20, 50);
    for(int i=0; i<shadowLen; i++)
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.addRoundedRect(shadowLen-i, shadowLen-i, this->width()-(shadowLen-i)*2, this->height()-(shadowLen-i)*2,3,3);

        color.setAlpha(160 - qSqrt(i)*50);
        painter.setPen(color);
        painter.drawPath(path);
    }
}

void MainWindow::on_btnClose_clicked()
{
    this->close();
}

void MainWindow::on_btnMin_clicked()
{
    this->showMinimized();
}

//move form
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton){
        dPos = event->pos();
        if(dPos.x()<this->width()-70||dPos.y()>30)
        {
            this->isMousePress=true;
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *)
{
    this->isMousePress=false;
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(isMousePress)
    {
        this->move(event->globalPos() - this->dPos);
        event->accept();
    }
}

void MainWindow::startRequest(QUrl url){
    reply = qnam.get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()),this, SLOT(httpFinished()));
    connect(reply, SIGNAL(readyRead()),this, SLOT(httpReadyRead()));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)),this, SLOT(updateDataReadProgress(qint64,qint64)));
}

void MainWindow::downloadFile(){
    QFileInfo fileInfo(url.path());
    this->fileName = fileInfo.fileName();
    if (fileName.isEmpty())
        fileName = "index.html";

    //todo 如果文件存在,就提示是否覆盖
    if (QFile::exists(fileName)) {
        QFile::remove(fileName);
    }

    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("HTTP"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(fileName).arg(file->errorString()));
        delete file;
        file = 0;
        return;
    }

    enableDownloadButton(false);
    ui->btnRun->setText("正在更新");
    // schedule the request
    ui->progUpdate->setValue(0);
    ui->progUpdate->show();
    httpRequestAborted = false;
    haveread=0;
    startRequest(url);
}

void MainWindow::cancelDownload(){
    ui->lblStatusTip->setText(tr("取消下载."));
    httpRequestAborted = true;
    reply->abort();
    this->enableDownloadButton(true);
}

void MainWindow::httpFinished(){
    if (httpRequestAborted) {
        if (file) {
            file->close();
            file->remove();
            delete file;
            file = 0;
        }
        reply->deleteLater();
        ui->progUpdate->hide();
        return;
    }
    ui->progUpdate->hide();
    file->flush();
    file->close();

    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error()) {
        file->remove();
        ui->lblResultTip->setText(tr("文件下载失败!"));
        ui->lblResultTip->setStyleSheet("color: yellow ;font-size:20px;background-image:url();background-color:transparent;border:0px;");
        if(QMessageBox::warning(this,tr("更新失败"),"请前往："+AppArgs[0]+"\r\n手动下载！",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)
        {
            QDesktopServices::openUrl(QUrl(AppArgs[0]));
        }
    } else if (!redirectionTarget.isNull()) {
        QUrl newUrl = url.resolved(redirectionTarget.toUrl());
        if (QMessageBox::question(this, tr("HTTP"),
                                  tr("Redirect to %1 ?").arg(newUrl.toString()),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            url = newUrl;
            reply->deleteLater();
            file->open(QIODevice::WriteOnly);
            file->resize(0);
            startRequest(url);
            return;
        }
    } else {
        QString fileName = url.fileName();
        ui->lblStatusTip->setText("程序更新完成,正在应用更用.");

        QDir currentDir = QDir::current();
        qDebug()<<currentDir.path();
        //currentDir.cdUp();
        //解压至指定文件夹
        JlCompress::extractDir(tr("%1\\%2").arg(QDir::currentPath()).arg(fileName),currentDir.path());

        //删除已下载文件
        if (QFile::exists(fileName))
        {
            QFile::remove(fileName);
        }
        ui->lblResultTip->setText("更新成功!");
        this->PostData();
    }
    ui->lblResultTip->show();
    ui->lblStatusTip->hide();

    ui->btnClose->show();
    ui->btnMin->show();
    ui->btnRun->setText("完成");
    isUpdated=true;
    this->enableDownloadButton(true);
    reply->deleteLater();
    reply = 0;
    delete file;
    file = 0;
}

void MainWindow::httpReadyRead(){
    if (file)
    {
        QByteArray bArr=reply->readAll();
        file->write(bArr);
    }
}

void MainWindow::updateDataReadProgress(qint64 bytesRead, qint64 totalBytes){
    if (httpRequestAborted)
        return;

    if (bytesRead<haveread)
        return;

    haveread=bytesRead;

    ui->progUpdate->setMaximum(totalBytes);
    ui->progUpdate->setValue(bytesRead);
    QString dotStr = "...";
    int value = ui->progUpdate->value();
    if(value%3==1)
    {
        dotStr=".. ";
    }
    else if(value%3==2)
    {
        dotStr=".  ";
    }
    ui->lblStatusTip->setText(QString("正在下载更新%1").arg(dotStr));
}

void MainWindow::enableDownloadButton(bool enabled){

    if(enabled)
    {
        ui->btnRun->setStyleSheet(tr("QPushButton {  border:1px sold black;  border-radius:18px;  background-image:url();  background-color:white;  width:160px;height:36px;font-size:14px;font-weight:bold;color:rgb(57, 134, 205);}QPushButton::hover {	background-color: snow;	color:green;}QPushButton::pressed {	background-color: white;}"));
    }else{
        ui->btnRun->setStyleSheet(tr("QPushButton {  border:1px sold black;  border-radius:18px;  background-image:url();  background-color:silver;  width:160px;height:36px;font-size:14px;font-weight:bold;color:Gray;}"));
    }
    ui->btnRun->setEnabled(enabled);
}

void MainWindow::slotAuthenticationRequired(QNetworkReply *, QAuthenticator *authenticator){
    authenticator->setUser(this->ConvertFromBase64(AppArgs[2]));
    authenticator->setPassword(this->ConvertFromBase64(AppArgs[3]));
}

#ifndef QT_NO_SSL
void MainWindow::sslErrors(QNetworkReply*,const QList<QSslError> &errors)
{
    QString errorString;
    foreach (const QSslError &error, errors) {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += error.errorString();
    }

    if (QMessageBox::warning(this, tr("HTTP"),
                             tr("One or more SSL errors has occurred: %1").arg(errorString),
                             QMessageBox::Ignore | QMessageBox::Abort) == QMessageBox::Ignore) {
        reply->ignoreSslErrors();
    }
}
#endif

void MainWindow::on_btnRun_clicked()
{
    if(isUpdated)
    {
        if(AppArgs.length()>4)
        {
            QProcess process;
            process.start(AppArgs[4],QStringList(),QIODevice::ReadWrite);
            //QThread::sleep(10);
            exit(0);
        }
        this->close();
        return;
    }

    this->ui->btnClose->hide();
    this->ui->btnMin->hide();
    url = QString(AppArgs[1].toUtf8());
    this->downloadFile();
}

QString MainWindow::ConvertFromBase64(QString &input)
{
    return tr(QByteArray::fromBase64(input.toUtf8()));
}

QString MainWindow::ReadConfig()
{
    QString fileName = QDir::currentPath()+"/update/Updater.config";
    qDebug()<<fileName;
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return "";
    //return file.readAll();
    QTextStream in(&file);
    QString line("");
    while (!in.atEnd()) {
        line = in.readLine();
    }
    return line;
}

void MainWindow::PostData()
{
    try{
        QNetworkAccessManager *netManager = new QNetworkAccessManager(this);

        QString url=ReadConfig()+"/api/sys/collect";
        qDebug()<<url;
        QString _content="_act=1&username="+AppArgs[4];
        QByteArray content = _content.toUtf8();
        int contentLength = content.length();
        QNetworkRequest req;
        req.setUrl(QUrl(url));
        req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
        req.setHeader(QNetworkRequest::ContentLengthHeader,contentLength);
        QNetworkReply* reply = netManager->post(req,content);

        qDebug()<<QLatin1String(reply->readAll());
    }
    catch(QString str)
    {
        qDebug()<<str;
    }
}
