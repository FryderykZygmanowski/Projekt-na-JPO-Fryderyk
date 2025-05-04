#ifndef STATION_H
#define STATION_H

#include <QString>
#include <QDateTime>

struct Commune {
    QString communeName;
    QString districtName;
    QString provinceName;
};

struct City {
    int id;
    QString name;
    Commune commune;
};

struct Address {
    int id;
    QString addressStreet;
};

struct Station {
    int id;
    QString stationName;
    double gegrLat;
    double gegrLon;
    City city;
    Address address;
};

struct SensorParam {
    QString paramName;
    QString paramFormula;
    QString paramCode;
    int idParam;
};

class Sensor {
public:
    int id;
    int stationId;
    struct Parameter {
        QString paramName;
        QString paramFormula;
        QString paramCode;
        int idParam;
    } param;

    double value;  // Wartość pomiaru
    QDateTime measurementTime;  // Czas pomiaru
};

// Struktura zawierająca dane o indeksie jakości powietrza
struct AirQualityIndex {
    int stationId;
    QDateTime stCalcDate;  // Data i czas obliczenia indeksu
    int stIndexLevel;      // Poziom indeksu (0-5)
    QString indexLevelName; // Nazwa poziomu indeksu
    QDateTime stSourceDataDate;  // Data zbierania danych
};

#endif // STATION_H
