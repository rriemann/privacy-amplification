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

    quint8 baseMask;

    if(isMaster) {
        baseMask = baseAMask;
    } else {
        baseMask = baseBMask;
    }
    quint8 bitMask = bitAMask;
    quint8 photonMask = photon01Mask;

    bool bit;
    bool valid;
    bool photon;
    for(qint64 i = 0; i < this->size(); i++) {
        this->getChar((char*)&measurementData);
        bit = measurementData & bitMask;
        if(isMaster) {
            valid = true;
        } else {
            photon = (bool)(measurementData & photon01Mask);
            valid = photon ^ ((bool)(measurementData & photon02Mask));
            bit = (!photon) ^ bit; // toggle Bob's bit in dependence of measured photon
        }
        Measurement measurement(measurementData & baseMask, bit, valid);
        measurements->append(measurement);
    }

    return measurements;
}
