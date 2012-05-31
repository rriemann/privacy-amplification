#ifndef FILE_H
#define FILE_H

#include <QFile>

#include "measurement.h"

class File : public QFile
{
    Q_OBJECT
public:
    explicit File(const QString &name, QObject *parent = 0);

    Measurements* getMeasurements(bool isMaster);

    static const quint8 baseAMask =    (1u << 1);
    static const quint8 bitAMask =     (1u << 0);
    static const quint8 baseBMask =    (1u << 3);
    static const quint8 bitBMask =     (1u << 2);
    static const quint8 photon01Mask = (1u << 4);
    static const quint8 photon02Mask = (1u << 5);

    
signals:
    
public slots:
    
};

#endif // FILE_H
