#include "file.h"

File::File(const QString &name, QObject *parent) :
    QFile(name, parent)
{
}
