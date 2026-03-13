#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QAction>

#include "velocitycanvas.h"
#include "midimanager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onRefreshDevices();
    void onMidiInputChanged(int index);
    void onMidiOutputChanged(int index);

    void onP1XChanged(double v);
    void onP1YChanged(double v);
    void onP2XChanged(double v);
    void onP2YChanged(double v);
    void onP1EnableChanged(bool en);
    void onP2EnableChanged(bool en);

    void onMinVelChanged(int v);
    void onMaxVelChanged(int v);
    void onMinInputChanged(int v);
    void onMaxInputChanged(int v);

    // Canvas drag → update spinboxes
    void onCanvasP1Changed(float x, float y);
    void onCanvasP2Changed(float x, float y);

    // MIDI input
    void onNoteOn(int channel, int note, int velocity);

    // File
    void onSave();
    void onLoad();

private:
    void buildUI();
    void populateDeviceCombos();
    void sendAnchorPoints();
    void sendVelocityRange();
    void sendInputRange();
    void clearHistory();

    float bezierCoord(float t, float p0, float p1c, float p2c, float p3) const;
    static float clampf(float v, float lo, float hi){ return v<lo?lo:(v>hi?hi:v); }

    // ---- Widgets ----
    VelocityCanvas  *m_canvas;
    MidiManager     *m_midi;

    // MIDI group
    QGroupBox       *m_grpMidi;
    QComboBox       *m_comboIn;
    QComboBox       *m_comboOut;
    QPushButton     *m_btnRefresh;

    // P1 group
    QGroupBox       *m_grpP1;
    QCheckBox       *m_chkP1;
    QDoubleSpinBox  *m_spnP1X;
    QDoubleSpinBox  *m_spnP1Y;

    // P2 group
    QGroupBox       *m_grpP2;
    QCheckBox       *m_chkP2;
    QDoubleSpinBox  *m_spnP2X;
    QDoubleSpinBox  *m_spnP2Y;

    // Input range group
    QGroupBox       *m_grpInput;
    QSpinBox        *m_spnMinInput;
    QSpinBox        *m_spnMaxInput;

    // Velocity range group
    QGroupBox       *m_grpVel;
    QSpinBox        *m_spnMinVel;
    QSpinBox        *m_spnMaxVel;

    // Menu
    QAction         *m_actSave;
    QAction         *m_actLoad;

    // State
    int  m_minVelocity = 1;
    int  m_maxVelocity = 127;
    int  m_minInput    = 0;
    int  m_maxInput    = 4095;

    bool m_blockSignals = false;
};

#endif // MAINWINDOW_H
