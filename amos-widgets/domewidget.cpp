#include "domewidget.h"

DomeWidget::DomeWidget(QWidget *parent):
    QWidget(parent),
    m_cover_position(0),
    m_cover_minimum(0),
    m_cover_maximum(400)
{
    this->setMinimumHeight(110);
    this->setMinimumWidth(250);
}

void DomeWidget::set_cover_position(int new_position) {
    this->m_cover_position = new_position;
    this->repaint();
}

void DomeWidget::set_cover_minimum(int new_minimum) {
    this->m_cover_minimum = new_minimum;
    if (this->m_cover_position < this->m_cover_minimum) {
        this->m_cover_position = this->m_cover_minimum;
    }
    this->repaint();
}

void DomeWidget::set_cover_maximum(int new_maximum) {
    this->m_cover_maximum = new_maximum;
    if (this->m_cover_position > this->m_cover_maximum) {
        this->m_cover_position = this->m_cover_maximum;
    }
    this->repaint();
}

void DomeWidget::paintEvent(QPaintEvent *e) {
    QPainter qp(this);
    qp.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    this->drawWidget(qp);
    QWidget::paintEvent(e);
}

void DomeWidget::drawWidget(QPainter &qp) {
    QColor black(0, 0, 0);
    QSize size(this->size());

    float angle = ((float) (this->m_cover_position - this->m_cover_minimum) / (float) (this->m_cover_maximum - this->m_cover_minimum)) * M_PI_2 * 0.9;
    float w = size.width() - 1;
    float h = size.height() - 1;
    float scale = (w < h ? w : h);

    qp.setPen(QPen(Qt::black, 0));

    QTransform transform;
    transform.translate(w / 2, h - 5);
    transform.scale(scale, scale);
    qp.setTransform(transform);

    // Lens
    qp.setBrush(QColor(120, 120, 255));
    qp.drawEllipse(QRectF(-0.05, -0.63, 0.1, 0.06));

    // Camera
    QLinearGradient camera(-0.1, 0, 0.1, 0);
    camera.setColorAt(0, QColor(26, 26, 26));
    camera.setColorAt(0.5, QColor(160, 160, 160));
    camera.setColorAt(1, QColor(210, 210, 210));
    qp.setBrush(camera);
    qp.drawRect(QRectF(-0.1, -0.6, 0.2, 0.55));

    QLinearGradient cover(0, 0, 0.4, 0);
    cover.setSpread(QLinearGradient::RepeatSpread);
    cover.setColorAt(0, QColor(99, 99, 99));
    cover.setColorAt(0.25, QColor(155, 155, 155));
    cover.setColorAt(0.75, QColor(150, 150, 150));
    cover.setColorAt(1, QColor(190, 190, 190));
    qp.setBrush(cover);

    QTransform left_wing(transform), right_wing(transform);
    qp.setTransform(left_wing.translate(-0.2, 0).rotate(-qRadiansToDegrees(angle)));
    qp.drawPolygon(QPolygonF(QVector<QPointF>{QPointF(0, 0), QPointF(0.2, 0), QPointF(0.2, -0.7), QPointF(0, -0.7)}));

    qp.setTransform(right_wing.translate(0.2, 0).rotate(qRadiansToDegrees(angle)));
    qp.drawPolygon(QPolygonF(QVector<QPointF>{QPointF(0, 0), QPointF(0, -0.72), QPointF(-0.4, -0.72), QPointF(-0.4, -0.7), QPointF(-0.2, -0.7), QPointF(-0.2, 0)}));
}
