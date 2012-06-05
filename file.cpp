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

    bool valid;
    for(qint64 i = 0; i < this->size(); i++) {
        this->getChar((char*)&measurementData);
        if(isMaster) {
            valid = true;
        } else {
            valid = !((measurementData & photon01Mask) == (measurementData & photon02Mask));
        }
        Measurement measurement(measurementData & baseMask,
                                measurementData & bitMask,
                                measurementData & photonMask,
                                valid);
        Q_ASSERT(measurement.valid);
        measurements->append(measurement);
    }

    return measurements;
}
