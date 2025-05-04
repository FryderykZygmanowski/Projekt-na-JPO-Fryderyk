#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QListWidgetItem>
#include <QVector>
#include "station.h"
#include "sensor.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onFetchButtonClicked();
    void onNetworkReply(QNetworkReply *reply);
    void onListItemClicked(QListWidgetItem *item);

private:
    enum class State {
        Idle,
        DisplayingStations,
        DisplayingSensors,
        DisplayingChart
    };

    enum class RequestType {
        Stations,
        Sensors,
        SensorData,
        AirQualityIndex
    };

    void parseStations(const QByteArray &data);
    void parseSensors(const QByteArray &data);
    void parseSensorData(const QByteArray &data);
    void onAirQualityIndexClicked();
    void parseAirQualityIndex(const QByteArray &data);

    Ui::MainWindow *ui;
    QNetworkAccessManager *manager;

    QVector<Station> stations;
    QVector<Sensor> sensors;

    State currentState = State::Idle;
    RequestType lastRequestType = RequestType::Stations;

    QWidget *chartWidget = nullptr;
};

#endif // MAINWINDOW_H
