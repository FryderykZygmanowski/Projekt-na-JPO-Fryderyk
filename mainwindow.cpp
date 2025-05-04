#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "station.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

#include <QPushButton>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStatusBar>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << "Start konstrukcji MainWindow";
    ui->setupUi(this);
    qDebug() << "setupUi() zakończone";

    auto *fetchButton = new QPushButton("Pobierz stacje", this);
    ui->verticalLayout->addWidget(fetchButton);
    qDebug() << "Dodano przycisk 'Pobierz stacje'";

    manager = new QNetworkAccessManager(this);
    qDebug() << "QNetworkAccessManager zainicjalizowany";

    connect(fetchButton, &QPushButton::clicked, this, &MainWindow::onFetchButtonClicked);
    connect(manager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);
    connect(ui->listWidget, &QListWidget::itemClicked, this, &MainWindow::onListItemClicked);

    auto *fetchAirQualityButton = new QPushButton("Pobierz Indeks Jakości Powietrza", this);
    ui->verticalLayout->addWidget(fetchAirQualityButton);
    connect(fetchAirQualityButton, &QPushButton::clicked, this, &MainWindow::onAirQualityIndexClicked);

    qDebug() << "MainWindow gotowy";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onFetchButtonClicked()
{
    qDebug() << "Kliknięto przycisk pobierania stacji";
    QString url = "https://api.gios.gov.pl/pjp-api/rest/station/findAll";
    QNetworkRequest request(url);
    lastRequestType = RequestType::Stations;
    manager->get(request);
}

void MainWindow::onListItemClicked(QListWidgetItem *item)
{
    int row = ui->listWidget->row(item);
    if (currentState == State::DisplayingStations && row >= 0 && row < stations.size()) {
        int stationId = stations[row].id;
        qDebug() << "Wybrano stację ID:" << stationId;
        QString url = QString("https://api.gios.gov.pl/pjp-api/rest/station/sensors/%1").arg(stationId);
        QNetworkRequest request(url);
        lastRequestType = RequestType::Sensors;
        manager->get(request);
    } else if (currentState == State::DisplayingSensors && row >= 0 && row < sensors.size()) {
        int sensorId = sensors[row].id;
        qDebug() << "Wybrano sensor ID:" << sensorId;
        QString url = QString("https://api.gios.gov.pl/pjp-api/rest/data/getData/%1").arg(sensorId);
        QNetworkRequest request(url);
        lastRequestType = RequestType::SensorData;
        manager->get(request);
    }
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Błąd sieci:" << reply->errorString();
        ui->statusbar->showMessage("Błąd sieci: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "Odpowiedź odebrana, typ zapytania:" << static_cast<int>(lastRequestType);

    switch (lastRequestType) {
    case RequestType::Stations:
        parseStations(data);
        break;
    case RequestType::Sensors:
        parseSensors(data);
        break;
    case RequestType::SensorData:
        parseSensorData(data);
        break;
    case RequestType::AirQualityIndex:
        parseAirQualityIndex(data);
        break;
    }

    reply->deleteLater();
}

void MainWindow::parseStations(const QByteArray &data)
{
    qDebug() << "Parsowanie stacji...";
    stations.clear();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();
    for (const auto &item : array) {
        QJsonObject obj = item.toObject();
        Station station;
        station.id = obj["id"].toInt();
        station.stationName = obj["stationName"].toString();
        station.gegrLat = obj["gegrLat"].toString().toDouble();
        station.gegrLon = obj["gegrLon"].toString().toDouble();
        QJsonObject cityObj = obj["city"].toObject();
        station.city.id = cityObj["id"].toInt();
        station.city.name = cityObj["name"].toString();
        QJsonObject communeObj = cityObj["commune"].toObject();
        station.city.commune.communeName = communeObj["communeName"].toString();
        station.city.commune.districtName = communeObj["districtName"].toString();
        station.city.commune.provinceName = communeObj["provinceName"].toString();
        stations.append(station);
    }

    ui->listWidget->clear();
    for (const auto &station : stations) {
        ui->listWidget->addItem(QString("ID: %1, %2, %3").arg(station.id).arg(station.stationName).arg(station.city.name));
    }

    currentState = State::DisplayingStations;
    qDebug() << "Wczytano stacje, liczba:" << stations.size();
}

void MainWindow::parseSensors(const QByteArray &data)
{
    qDebug() << "Parsowanie sensorów...";
    sensors.clear();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();
    for (const auto &item : array) {
        QJsonObject obj = item.toObject();
        Sensor sensor;
        sensor.id = obj["id"].toInt();
        sensor.stationId = obj["stationId"].toInt();
        QJsonObject param = obj["param"].toObject();
        sensor.param.paramName = param["paramName"].toString();
        sensor.param.paramFormula = param["paramFormula"].toString();
        sensor.param.paramCode = param["paramCode"].toString();
        sensor.param.idParam = param["idParam"].toInt();
        sensors.append(sensor);
    }

    ui->listWidget->clear();
    for (const auto &sensor : sensors) {
        ui->listWidget->addItem(QString("%1 (%2)").arg(sensor.param.paramName).arg(sensor.param.paramFormula));
    }

    currentState = State::DisplayingSensors;
    qDebug() << "Wczytano sensory, liczba:" << sensors.size();
}
void MainWindow::onAirQualityIndexClicked()
{
    if (currentState == State::DisplayingStations) {
        QListWidgetItem *selectedItem = ui->listWidget->currentItem();
        if (!selectedItem) {
            ui->statusbar->showMessage("Wybierz stację, aby pobrać indeks jakości powietrza.");
            return;
        }

        int row = ui->listWidget->row(selectedItem);
        if (row >= 0 && row < stations.size()) {
            int stationId = stations[row].id;
            QString url = QString("https://api.gios.gov.pl/pjp-api/rest/aqindex/getIndex/%1").arg(stationId);
            QNetworkRequest request(url);
            lastRequestType = RequestType::AirQualityIndex;
            manager->get(request);
            ui->statusbar->showMessage("Pobieranie indeksu jakości powietrza...");
        }
    } else {
        ui->statusbar->showMessage("Najpierw wybierz stację.");
    }
}
void MainWindow::parseSensorData(const QByteArray &data)
{
    qDebug() << "Parsowanie danych z sensora...";
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QJsonArray values = obj["values"].toArray();

    QLineSeries *series = new QLineSeries();

    double sum = 0;
    int count = 0;
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    QDateTime minDate, maxDate;

    QVector<double> valueList;
    QVector<qint64> timeList;

    for (const auto &val : values) {
        QJsonObject valObj = val.toObject();
        double value = valObj["value"].toDouble();
        QString dateStr = valObj["date"].toString();
        QDateTime dt = QDateTime::fromString(dateStr, Qt::ISODate);

        if (dt.isValid() && !qIsNaN(value)) {
            qint64 msec = dt.toMSecsSinceEpoch();
            series->append(msec, value);
            sum += value;
            count++;

            if (value < minValue) {
                minValue = value;
                minDate = dt;
            }
            if (value > maxValue) {
                maxValue = value;
                maxDate = dt;
            }

            valueList.append(value);
            timeList.append(msec);
        }
    }

    // Średnia i trend
    double avg = count > 0 ? sum / count : 0;
    QString trend = "Brak danych";
    if (valueList.size() >= 2) {
        double slope = (valueList.last() - valueList.first()) / (timeList.last() - timeList.first());
        if (slope > 0) trend = "Rosnący";
        else if (slope < 0) trend = "Malejący";
        else trend = "Stabilny";
    }

    // Wykres
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Dane pomiarowe");
    chart->legend()->hide();

    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("dd.MM hh:mm");
    axisX->setTitleText("Czas");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Wartość");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    if (chartWidget) {
        ui->verticalLayout->removeWidget(chartWidget);
        delete chartWidget;
    }
    chartWidget = chartView;
    ui->verticalLayout->addWidget(chartWidget);

    currentState = State::DisplayingChart;

    QString analysisInfo = QString("Min: %1 (%2)\nMax: %3 (%4)\nŚrednia: %5\nTrend: %6")
                               .arg(minValue).arg(minDate.toString())
                               .arg(maxValue).arg(maxDate.toString())
                               .arg(avg)
                               .arg(trend);

    ui->statusbar->showMessage(analysisInfo);
    qDebug() << "Analiza danych:" << analysisInfo;
}

void MainWindow::parseAirQualityIndex(const QByteArray &data)
{
    qDebug() << "Parsowanie indeksu jakości powietrza...";
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    AirQualityIndex airQualityIndex;
    airQualityIndex.stationId = obj["stationId"].toInt();
    airQualityIndex.stCalcDate = QDateTime::fromString(obj["stCalcDate"].toString(), Qt::ISODate);
    airQualityIndex.stIndexLevel = obj["stIndexLevel"].toObject()["id"].toInt();
    airQualityIndex.indexLevelName = obj["stIndexLevel"].toObject()["indexLevelName"].toString();
    airQualityIndex.stSourceDataDate = QDateTime::fromString(obj["stSourceDataDate"].toString(), Qt::ISODate);

    QString airQualityInfo = QString("Indeks jakości powietrza dla stacji %1:\n").arg(airQualityIndex.stationId);
    airQualityInfo += QString("Obliczono: %1\n").arg(airQualityIndex.stCalcDate.toString());
    airQualityInfo += QString("Poziom: %1 - %2\n").arg(airQualityIndex.stIndexLevel).arg(airQualityIndex.indexLevelName);
    airQualityInfo += QString("Data zbierania danych: %1\n").arg(airQualityIndex.stSourceDataDate.toString());

    ui->statusbar->showMessage(airQualityInfo);
    qDebug() << "Indeks jakości powietrza przetworzony.";
}
