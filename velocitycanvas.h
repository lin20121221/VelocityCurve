#ifndef VELOCITYCANVAS_H
#define VELOCITYCANVAS_H

#include <QWidget>
#include <QPointF>
#include <QVector>
#include <QSizePolicy>

class VelocityCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit VelocityCanvas(QWidget *parent = nullptr);

    void setP1(float x, float y);
    void setP2(float x, float y);
    void setP1Enabled(bool en);
    void setP2Enabled(bool en);

    QPointF p1() const { return m_p1; }
    QPointF p2() const { return m_p2; }
    bool p1Enabled() const { return m_p1Enabled; }
    bool p2Enabled() const { return m_p2Enabled; }

    void addHistoryPoint(float velT, float outVal);
    void clearHistory();
    void setCurrentPoint(float velT, float outVal);
    void clearCurrentPoint();

    QSize sizeHint() const override { return QSize(480, 480); }
    QSize minimumSizeHint() const override { return QSize(200, 200); }

signals:
    void p1Changed(float x, float y);
    void p2Changed(float x, float y);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    static constexpr float MIDI_MAX = 127.0f;

    QPointF m_p1 { 52,  31 };
    QPointF m_p2 { 76,  81 };
    bool    m_p1Enabled = true;
    bool    m_p2Enabled = true;

    // Store history in MIDI coords so it survives resize
    struct MidiDot { float t; float val; };
    QVector<MidiDot> m_history;
    bool    m_hasCurrentPoint = false;
    MidiDot m_currentPoint    {};

    int m_dragging = -1;

    float   canvasSize() const;
    QPointF midiToPixel(float mx, float my) const;
    QPointF pixelToMidi(float px, float py) const;
    static float bezierCoord(float t, float p0, float cp1, float cp2, float p3);
    static float clampf(float v, float lo, float hi) { return v<lo?lo:(v>hi?hi:v); }
};

#endif // VELOCITYCANVAS_H
