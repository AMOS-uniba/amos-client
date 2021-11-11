#include "formatters.h"


QColor Formatters::linear_interpolate(QColor first, double stop1, QColor second, double stop2, double value) {
    const double x = (value - stop1) / (stop2 - stop1);
    return QColor::fromRgbF(
        first.redF() * (1 - x) + second.redF() * x,
        first.greenF() * (1 - x) + second.greenF() * x,
        first.blueF() * (1 - x) + second.blueF() * x
    );
}

QColor Formatters::piecewise_linear_interpolator(const QVector<Node> & nodes, double position) {
    if (position < nodes.first().position) {
        return nodes.first().colour;
    }

    for (QVector<Node>::const_iterator node = nodes.cbegin(); node + 1 != nodes.cend(); ++node) {
        if ((node->position <= position) && (position <= (node + 1)->position)) {
            const Node & left = *node;
            const Node & right = *(node + 1);
            return Formatters::linear_interpolate(left.colour, left.position, right.colour, right.position, position);
        }
    }

    return nodes.last().colour;
}

QColor Formatters::altitude_colour(double altitude) {
    return Formatters::piecewise_linear_interpolator({
        {-90.0, QColor::fromRgbF(0.0, 0.0, 0.0)},
        {-18.0, QColor::fromRgbF(0.0, 0.0, 0.25)},
        {  0.0, QColor::fromRgbF(0.0, 0.0, 1.0)},
        {  0.0, QColor::fromRgbF(1.0, 0.25, 0.0)},
        {  4.0, QColor::fromRgbF(1.0, 0.75, 0.0)},
        {  8.0, QColor::fromRgbF(0.0, 0.75, 1.0)},
        { 90.0, QColor::fromRgbF(0.0, 0.62, 0.87)}
    }, altitude);
}

QString Formatters::format_duration(unsigned int duration) {
    const unsigned int days = duration / 86400;
    const unsigned int hours = (duration % 86400) / 3600;
    const unsigned int minutes = (duration % 3600) / 60;
    const unsigned int seconds = duration % 60;

    return QString("%1d %2:%3:%4")
        .arg(days)
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

QString Formatters::format_duration_double(double seconds, unsigned int places) {
    const unsigned int duration = (unsigned int) seconds;
    const unsigned int d = duration / 86400;
    const unsigned int h = (duration % 86400) / 3600;
    const unsigned int m = (duration % 3600) / 60;
    const double s = fmod(seconds, 60.0);

    const QString hhmmss = QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, places + 3, 'f', places, QChar('0'));

    return (d == 0) ? hhmmss : QString("%1d %2").arg(d).arg(hhmmss);
}
