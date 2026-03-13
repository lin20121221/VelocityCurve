#include "velocitycanvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QResizeEvent>
#include <cmath>

VelocityCanvas::VelocityCanvas(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(false);
    // Keep aspect ratio square by setting a minimum size
    setMinimumSize(200, 200);
}

// Use the smaller of width/height to keep the canvas square
float VelocityCanvas::canvasSize() const
{
    return (float)qMin(width(), height());
}

// MIDI [0,127] -> pixel, y-axis flipped
QPointF VelocityCanvas::midiToPixel(float mx, float my) const
{
    float s = canvasSize();
    return QPointF(mx / MIDI_MAX * s, (1.0f - my / MIDI_MAX) * s);
}

// pixel -> MIDI [0,127]
QPointF VelocityCanvas::pixelToMidi(float px, float py) const
{
    float s = canvasSize();
    return QPointF(px / s * MIDI_MAX, (1.0f - py / s) * MIDI_MAX);
}

float VelocityCanvas::bezierCoord(float t, float p0, float cp1, float cp2, float p3)
{
    float u = 1.0f - t;
    return u*u*u*p0 + 3*u*u*t*cp1 + 3*u*t*t*cp2 + t*t*t*p3;
}

void VelocityCanvas::setP1(float x, float y)
{
    m_p1 = QPointF(clampf(x, 0, MIDI_MAX), clampf(y, 0, MIDI_MAX));
    update();
}

void VelocityCanvas::setP2(float x, float y)
{
    m_p2 = QPointF(clampf(x, 0, MIDI_MAX), clampf(y, 0, MIDI_MAX));
    update();
}

void VelocityCanvas::setP1Enabled(bool en) { m_p1Enabled = en; update(); }
void VelocityCanvas::setP2Enabled(bool en) { m_p2Enabled = en; update(); }

void VelocityCanvas::addHistoryPoint(float velT, float outVal)
{
    m_history.append({velT, outVal});
    update();
}

void VelocityCanvas::clearHistory()
{
    m_history.clear();
    m_hasCurrentPoint = false;
    update();
}

void VelocityCanvas::setCurrentPoint(float velT, float outVal)
{
    m_currentPoint = {velT, outVal};
    m_hasCurrentPoint = true;
    update();
}

void VelocityCanvas::clearCurrentPoint()
{
    m_hasCurrentPoint = false;
    update();
}

void VelocityCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void VelocityCanvas::paintEvent(QPaintEvent *)
{
    QPainter g(this);
    g.setRenderHint(QPainter::Antialiasing, true);

    float s = canvasSize();

    // White background (square area)
    g.fillRect(QRectF(0, 0, s, s), QColor("#ffffff"));

    // ------ Grid lines ------
    QPen gridPen(QColor(220, 220, 220), 1, Qt::DashLine);
    g.setPen(gridPen);
    QFont smallFont = g.font();
    smallFont.setPixelSize(qMax(9, (int)(s * 0.025f)));
    g.setFont(smallFont);

    for (int v = 0; v <= 127; v += 16) {
        float px = v / MIDI_MAX * s;
        float py = (1.0f - v / MIDI_MAX) * s;
        g.drawLine(QPointF(px, 0), QPointF(px, s));  // vertical
        g.drawLine(QPointF(0, py), QPointF(s,  py)); // horizontal
        // Y-axis labels
        g.setPen(QColor(140, 140, 140));
        if (v > 0)
            g.drawText(QRectF(2, py - 12, 30, 12), Qt::AlignLeft | Qt::AlignVCenter,
                       QString::number(v));
        g.setPen(gridPen);
    }

    // ------ Diagonal reference line ------
    g.setPen(QPen(QColor(200, 200, 200), 1));
    g.drawLine(midiToPixel(0, 0), midiToPixel(127, 127));

    // ------ Control points pixel coords ------
    QPointF start = midiToPixel(0,   0);
    QPointF end   = midiToPixel(127, 127);
    QPointF cp1   = midiToPixel((float)m_p1.x(), (float)m_p1.y());
    QPointF cp2   = midiToPixel((float)m_p2.x(), (float)m_p2.y());

    // ------ Bezier curve ------
    QPainterPath path;
    path.moveTo(start);
    if (m_p1Enabled && m_p2Enabled)
        path.cubicTo(cp1, cp2, end);
    else if (m_p1Enabled)
        path.cubicTo(cp1, cp1, end);
    else if (m_p2Enabled)
        path.cubicTo(cp2, cp2, end);
    else
        path.lineTo(end);

    g.setPen(QPen(Qt::black, qMax(1.5, s * 0.004)));
    g.setBrush(Qt::NoBrush);
    g.drawPath(path);

    // ------ Guide lines & handles ------
    float handleR = qMax(4.0, (double)(s * 0.012));
    QPen dashPen(QColor(130, 130, 130), 1, Qt::DashLine);

    if (m_p1Enabled) {
        g.setPen(dashPen);
        g.drawLine(start, cp1);
        g.setPen(Qt::NoPen);
        g.setBrush(QColor(100, 100, 100));
        g.drawEllipse(cp1, handleR, handleR);
    }
    if (m_p2Enabled) {
        g.setPen(dashPen);
        g.drawLine(end, cp2);
        g.setPen(Qt::NoPen);
        g.setBrush(QColor(100, 100, 100));
        g.drawEllipse(cp2, handleR, handleR);
    }

    // ------ History dots (green) ------
    float dotR = qMax(2.5, (double)(s * 0.007));
    g.setPen(Qt::NoPen);
    g.setBrush(Qt::green);
    for (const MidiDot &d : m_history) {
        QPointF px = midiToPixel(d.t * MIDI_MAX, d.val);
        g.drawEllipse(px, dotR, dotR);
    }

    // ------ Current point (red) ------
    if (m_hasCurrentPoint) {
        g.setBrush(Qt::red);
        QPointF px = midiToPixel(m_currentPoint.t * MIDI_MAX, m_currentPoint.val);
        g.drawEllipse(px, dotR + 1.5, dotR + 1.5);
    }

    // ------ Border ------
    g.setPen(QPen(QColor(160, 160, 160), 1));
    g.setBrush(Qt::NoBrush);
    g.drawRect(QRectF(0, 0, s - 1, s - 1));
}

void VelocityCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    QPointF mouse = event->pos();
    QPointF cp1 = midiToPixel((float)m_p1.x(), (float)m_p1.y());
    QPointF cp2 = midiToPixel((float)m_p2.x(), (float)m_p2.y());
    float s = canvasSize();
    float threshold = qMax(10.0f, s * 0.03f);

    auto dist = [](QPointF a, QPointF b) {
        return (float)std::sqrt(std::pow(a.x()-b.x(),2)+std::pow(a.y()-b.y(),2));
    };

    if (m_p1Enabled && dist(mouse, cp1) < threshold)
        m_dragging = 0;
    else if (m_p2Enabled && dist(mouse, cp2) < threshold)
        m_dragging = 1;
}

void VelocityCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || m_dragging < 0) return;

    float s = canvasSize();
    float px = clampf((float)event->x(), 0, s);
    float py = clampf((float)event->y(), 0, s);
    QPointF midi = pixelToMidi(px, py);
    float mx = clampf((float)midi.x(), 0, MIDI_MAX);
    float my = clampf((float)midi.y(), 0, MIDI_MAX);

    if (m_dragging == 0 && m_p1Enabled) {
        m_p1 = QPointF(mx, my);
        emit p1Changed(mx, my);
    } else if (m_dragging == 1 && m_p2Enabled) {
        m_p2 = QPointF(mx, my);
        emit p2Changed(mx, my);
    }
    update();
}

void VelocityCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = -1;
}
