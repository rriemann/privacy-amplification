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

    bool bit;
    bool valid;
    bool photon;
    for(qint64 i = 0; i < this->size(); i++) {
        this->getChar((char*)&measurementData);
        if(isMaster) {
            valid = true;
            bit = measurementData & bitAMask;
        } else {
            // bool base = (bool)(measurementData & baseBMask);
            photon = (bool)(measurementData & photon01Mask);
            valid = photon ^ ((bool)(measurementData & photon02Mask));
            bit =  !photon;
        }
        Measurement *measurement = new Measurement(measurementData & baseMask, bit, valid);
        measurements->append(measurement);
        /*
        if(i >= 200)
            break;
        */
    }

    return measurements;
}
