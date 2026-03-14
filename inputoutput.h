#ifndef INPUTOUTPUT_H
#define INPUTOUTPUT_H

#include <QString>
#include <QWidget>

namespace IOHelper {
    
    QString importPointCloud(QWidget* parent);
    QString importImage(QWidget* parent);
    QString exportGISData(QWidget* parent);

}

#endif // INPUTOUTPUT_H