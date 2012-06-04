#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <QPair>
#include <QList>
#include <QMetaType>

struct Measurement
{
    Measurement(bool base, bool bit, bool photon = 0) : base(base), bit(bit), photon(photon) {}
    explicit Measurement() : base(0), bit(0), photon(0) {}
    Measurement &operator=(const Measurement &other)
    {base = other.base; bit = other.bit; photon = other.photon; return *this;}

    bool base;
    bool bit;
    bool photon;
};

inline QDataStream& operator>>(QDataStream& stream, Measurement& measurement)
{
    stream >> measurement.base >> measurement.bit >> measurement.photon;
    return stream;
}

inline QDataStream& operator<<(QDataStream& stream, const Measurement& measurement)
{
    stream << measurement.base << measurement.bit << measurement.photon;
    return stream;
}


typedef QList<Measurement> Measurements;
Q_DECLARE_METATYPE(Measurements)
static int id3 = qRegisterMetaType<Measurements>();
static int id4 = qRegisterMetaTypeStreamOperators<Measurements>();

#endif // MEASUREMENT_H
