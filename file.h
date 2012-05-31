#ifndef FILE_H
#define FILE_H

#include <QFile>

class File : public QFile
{
    Q_OBJECT
public:
    explicit File(const QString &name, QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // FILE_H
