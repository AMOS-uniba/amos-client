#ifndef FORMATTERS_H
#define FORMATTERS_H

#include "forward.h"

namespace Formatters {
    struct Node {
        double position;
        QColor colour;
    };

    QColor linear_interpolate(QColor first, double stop1, QColor second, double stop2, double value);
    QColor piecewise_linear_interpolator(const QVector<Node> & nodes, double position);
    QColor altitude_colour(double altitude);

    QString format_duration(unsigned int seconds);
    QString format_duration_double(double seconds, unsigned int places = 0);
};

#endif // FORMATTERS_H
