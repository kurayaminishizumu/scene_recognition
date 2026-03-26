#ifndef INPUTOUTPUT_H
#define INPUTOUTPUT_H

#include <QString>
#include <QWidget>

namespace IOHelper {
    QString importImage(QWidget* parent);
    QString exportGeoJSON(QWidget* parent);

}

#endif // INPUTOUTPUT_H