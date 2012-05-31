#include "file.h"

File::File(const QString &name, QObject *parent) :
    QFile(name, parent)
{
}

Measurements *File::getMeasurements(bool isMaster)
{
    this->seek(0);

    Measurements* measurements = new Measurements;
    quint8 measurementData = 0;

    quint8 baseMask, bitMask;

    if(isMaster) {
        baseMask = baseAMask;
        bitMask = bitAMask;
    } else {
        baseMask = baseBMask;
        bitMask = bitBMask;
    }
    quint8 photonMask = photon01Mask;

    for(qint64 i = 0; i < this->size(); i++) {
        this->getChar((char*)&measurementData);
        Measurement measurement(measurementData & baseMask, measurementData & bitMask, measurementData & photonMask);
        measurements->append(measurement);
    }

    return measurements;
}
