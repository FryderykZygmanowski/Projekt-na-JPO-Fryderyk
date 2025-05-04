#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), manager(new QNetworkAccessManager(this)) {
    ui->setupUi(this);
    qDebug() << "SSL available:" << QSslSocket::supportsSsl();
    qDebug() << "SSL version:" << QSslSocket::sslLibraryVersionString();

    listaStacji = new QListWidget(this);
    setCentralWidget(listaStacji);
    connect(manager, &QNetworkAccessManager::finished, this, &MainWindow::obsluzOdpowiedzStacje);
    listaStacji->setStyleSheet("color: white; background-color: black;");
    pobierzStacjePomiarowe();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::pobierzStacjePomiarowe() {
    qDebug() << "Wywołano pobierzStacjePomiarowe";
    QUrl url("https://api.gios.gov.pl/pjp-api/rest/station/findAll");
    QNetworkRequest request(url);
    manager->get(request);
}


void MainWindow::obsluzOdpowiedzStacje(QNetworkReply *reply) {
    qDebug() << "Wywołano obsluzOdpowiedzStacje";
    if (reply->error()) {
        listaStacji->addItem("Błąd pobierania danych");
        reply->deleteLater();
        qDebug() << "Błąd sieci: " << reply->errorString();
        return;
    }
    qDebug() << "Status odpowiedzi:" << reply->errorString();
    qDebug() << "Kod odpowiedzi HTTP:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Treść odpowiedzi:" << QString::fromUtf8(reply->readAll());
    return;

    QByteArray responseData = reply->readAll();
    qDebug() << "Dane z API:";
    qDebug() << responseData;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (jsonDoc.isArray()) {
        QJsonArray array = jsonDoc.array();
        for (const QJsonValue &value : array) {
            QJsonObject obj = value.toObject();
            QString nazwa = obj["stationName"].toString();
            listaStacji->addItem(nazwa);
        }
    }
    reply->deleteLater();
}

