#include "qdomewidget.h"

QDomeWidget::QDomeWidget(QWidget * parent):
    QWidget(parent),
    m_cover_position(0),
    m_cover_minimum(0),
    m_cover_maximum(400)
{
    this->setMinimumHeight(140);
    this->setMinimumWidth(250);
}

void QDomeWidget::set_cover_position(int new_position) {
    this->m_cover_position = new_position;
    this->repaint();
}

void QDomeWidget::set_cover_minimum(int new_minimum) {
    this->m_cover_minimum = new_minimum;
    if (this->m_cover_position < this->m_cover_minimum) {
        this->m_cover_position = this->m_cover_minimum;
    }
    this->repaint();
}

void QDomeWidget::set_cover_maximum(int new_maximum) {
    this->m_cover_maximum = new_maximum;
    if (this->m_cover_position > this->m_cover_maximum) {
        this->m_cover_position = this->m_cover_maximum;
    }
    this->repaint();
}

void QDomeWidget::paintEvent(QPaintEvent * e) {
    QPainter qp(this);
    qp.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    this->drawWidget(qp);
    QWidget::paintEvent(e);
}

void QDomeWidget::drawWidget(QPainter & qp) {
    QColor black(0, 0, 0);
    QSize size(this->size());

    double CameraWidth = 0.2;
    double CameraHeight = 0.55;
    double BoxWidth = 0.36;
    double BoxHeight = 0.25;
    double CoverWidth = 0.36;
    double CoverHeight = 0.65;
    double TopLidThickness = 0.02;

    float angle = ((float) (this->m_cover_position - this->m_cover_minimum) / (float) (this->m_cover_maximum - this->m_cover_minimum)) * M_PI_2 * 0.9;
    float w = size.width() - 1;
    float h = size.height() - 1;
    float scale = ((w * 2) < h ? (w * 2) : h);

    qp.setPen(QPen(Qt::black, 0));

    QTransform transform;
    transform.translate(w / 2, h * 0.75 - 1);
    transform.scale(scale, scale);
    qp.setTransform(transform);

    // Lens
    qp.setBrush(QColor(120, 120, 255));
    qp.drawEllipse(QRectF(-0.05, -CameraHeight - 0.03, 0.1, 0.06));

    // Camera
    QLinearGradient camera(-CameraWidth / 2, 0, CameraWidth / 2, 0);
    camera.setColorAt(0, QColor(86, 86, 86));
    camera.setColorAt(0.5, QColor(160, 160, 160));
    camera.setColorAt(1, QColor(230, 230, 230));
    qp.setBrush(camera);
    qp.drawRect(QRectF(-CameraWidth / 2, -CameraHeight, CameraWidth, CameraHeight));

    qp.setBrush(QColor(195, 195, 195));
    qp.drawRect(QRectF(-BoxWidth / 2.0, 0, BoxWidth, BoxHeight));

    QPixmap logo(":/images/blue.ico");
    qp.drawPixmap(QRectF(-0.075, 0.05, 0.15, 0.15), logo.scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation), QRectF(0, 0, 25, 25));

    QLinearGradient cover(0, 0, 0.4, 0);
    cover.setSpread(QLinearGradient::RepeatSpread);
    cover.setColorAt(0, QColor(155, 155, 155));
    cover.setColorAt(0.25, QColor(195, 195, 195));
    cover.setColorAt(0.75, QColor(190, 190, 190));
    cover.setColorAt(1, QColor(220, 220, 220));
    qp.setBrush(cover);

    QTransform left_wing(transform), right_wing(transform);
    qp.setTransform(left_wing.translate(-CoverWidth / 2, 0).rotate(-qRadiansToDegrees(angle)));
    qp.drawPolygon(QPolygonF(QVector<QPointF>{
                                 QPointF(0, 0),
                                 QPointF(CoverWidth / 2, 0),
                                 QPointF(CoverWidth / 2, -CoverHeight + TopLidThickness),
                                 QPointF(0, -CoverHeight + TopLidThickness)
                             }));

    qp.setTransform(right_wing.translate(CoverWidth / 2, 0).rotate(qRadiansToDegrees(angle)));
    qp.drawPolygon(QPolygonF(QVector<QPointF>{
                                 QPointF(0, 0),
                                 QPointF(0, -CoverHeight),
                                 QPointF(-CoverWidth, -CoverHeight),
                                 QPointF(-CoverWidth, -CoverHeight + TopLidThickness),
                                 QPointF(-CoverWidth / 2, -CoverHeight + TopLidThickness),
                                 QPointF(-CoverWidth / 2, 0)
                             }));
}
