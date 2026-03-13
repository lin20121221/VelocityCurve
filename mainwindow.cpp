#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QtMath>

// ──────────────────────────────────────────────
//  Constructor / Destructor
// ──────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_midi(new MidiManager(this))
{
    setWindowTitle("Velocity Curve");
    buildUI();
    populateDeviceCombos();

    // Wire canvas drag → spinbox update
    connect(m_canvas, &VelocityCanvas::p1Changed, this, &MainWindow::onCanvasP1Changed);
    connect(m_canvas, &VelocityCanvas::p2Changed, this, &MainWindow::onCanvasP2Changed);

    // Wire MIDI note
    connect(m_midi, &MidiManager::noteOn, this, &MainWindow::onNoteOn);
}

MainWindow::~MainWindow()
{
    // MidiManager RAII closes devices
}

// ──────────────────────────────────────────────
//  UI Construction
// ──────────────────────────────────────────────
static QDoubleSpinBox *makeMidiSpinBox(double val = 0)
{
    auto *s = new QDoubleSpinBox;
    s->setRange(0, 127);
    s->setDecimals(0);
    s->setValue(val);
    s->setFixedWidth(80);
    return s;
}

static QSpinBox *makeSpinBox(int lo, int hi, int val)
{
    auto *s = new QSpinBox;
    s->setRange(lo, hi);
    s->setValue(val);
    s->setFixedWidth(90);
    return s;
}

void MainWindow::buildUI()
{
    // ── Menu ──────────────────────────────────
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    m_actSave = fileMenu->addAction(tr("Save"));
    m_actLoad = fileMenu->addAction(tr("Load"));
    connect(m_actSave, &QAction::triggered, this, &MainWindow::onSave);
    connect(m_actLoad, &QAction::triggered, this, &MainWindow::onLoad);

    // ── Central Widget ────────────────────────
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(10);

    // ── Canvas (left) ─────────────────────────
    m_canvas = new VelocityCanvas(this);
    m_canvas->setP1(52, 31);
    m_canvas->setP2(76, 81);
    rootLayout->addWidget(m_canvas, 1);

    // ── Right Panel ───────────────────────────
    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->setSpacing(8);
    rootLayout->addLayout(rightLayout, 0);

    // ── MIDI Devices group ────────────────────
    m_grpMidi = new QGroupBox(tr("MIDI Devices"));
    QGridLayout *midiGrid = new QGridLayout(m_grpMidi);
    m_comboIn   = new QComboBox; m_comboIn->setMinimumWidth(220);
    m_comboOut  = new QComboBox; m_comboOut->setMinimumWidth(220);
    m_btnRefresh= new QPushButton(tr("Refresh"));
    midiGrid->addWidget(new QLabel("IN"),   0, 0);
    midiGrid->addWidget(m_comboIn,          0, 1);
    midiGrid->addWidget(new QLabel("OUT"),  1, 0);
    midiGrid->addWidget(m_comboOut,         1, 1);
    midiGrid->addWidget(m_btnRefresh,       2, 1, Qt::AlignLeft);
    rightLayout->addWidget(m_grpMidi);

    connect(m_btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshDevices);
    connect(m_comboIn,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMidiInputChanged);
    connect(m_comboOut, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMidiOutputChanged);

    // ── Input Range group ─────────────────────
    m_grpInput = new QGroupBox(tr("Input (0~4095)"));
    QGridLayout *inputGrid = new QGridLayout(m_grpInput);
    m_spnMinInput = makeSpinBox(0, 4095, 0);
    m_spnMaxInput = makeSpinBox(0, 4095, 4095);
    inputGrid->addWidget(new QLabel("Min"), 0, 0);
    inputGrid->addWidget(m_spnMinInput,     0, 1);
    inputGrid->addWidget(new QLabel("Max"), 1, 0);
    inputGrid->addWidget(m_spnMaxInput,     1, 1);
    rightLayout->addWidget(m_grpInput);

    connect(m_spnMinInput, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onMinInputChanged);
    connect(m_spnMaxInput, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onMaxInputChanged);

    // ── Velocity Range group ──────────────────
    m_grpVel = new QGroupBox(tr("Velocity (1~127)"));
    QGridLayout *velGrid = new QGridLayout(m_grpVel);
    m_spnMinVel = makeSpinBox(1, 127, 1);
    m_spnMaxVel = makeSpinBox(1, 127, 127);
    velGrid->addWidget(new QLabel("Min"), 0, 0);
    velGrid->addWidget(m_spnMinVel,       0, 1);
    velGrid->addWidget(new QLabel("Max"), 1, 0);
    velGrid->addWidget(m_spnMaxVel,       1, 1);
    rightLayout->addWidget(m_grpVel);

    connect(m_spnMinVel, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onMinVelChanged);
    connect(m_spnMaxVel, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onMaxVelChanged);

    // ── P1 group ──────────────────────────────
    m_grpP1 = new QGroupBox;
    QHBoxLayout *p1lay = new QHBoxLayout(m_grpP1);
    m_chkP1  = new QCheckBox("P1"); m_chkP1->setChecked(true);
    m_spnP1X = makeMidiSpinBox(52);
    m_spnP1Y = makeMidiSpinBox(31);
    p1lay->addWidget(m_chkP1);
    p1lay->addWidget(new QLabel("X")); p1lay->addWidget(m_spnP1X);
    p1lay->addWidget(new QLabel("Y")); p1lay->addWidget(m_spnP1Y);
    rightLayout->addWidget(m_grpP1);

    connect(m_chkP1,  &QCheckBox::toggled,   this, &MainWindow::onP1EnableChanged);
    connect(m_spnP1X, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onP1XChanged);
    connect(m_spnP1Y, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onP1YChanged);

    // ── P2 group ──────────────────────────────
    m_grpP2 = new QGroupBox;
    QHBoxLayout *p2lay = new QHBoxLayout(m_grpP2);
    m_chkP2  = new QCheckBox("P2"); m_chkP2->setChecked(true);
    m_spnP2X = makeMidiSpinBox(76);
    m_spnP2Y = makeMidiSpinBox(81);
    p2lay->addWidget(m_chkP2);
    p2lay->addWidget(new QLabel("X")); p2lay->addWidget(m_spnP2X);
    p2lay->addWidget(new QLabel("Y")); p2lay->addWidget(m_spnP2Y);
    rightLayout->addWidget(m_grpP2);

    connect(m_chkP2,  &QCheckBox::toggled,   this, &MainWindow::onP2EnableChanged);
    connect(m_spnP2X, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onP2XChanged);
    connect(m_spnP2Y, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onP2YChanged);

    rightLayout->addStretch();
    resize(900, 600);
}

// ──────────────────────────────────────────────
//  Device helpers
// ──────────────────────────────────────────────
static const QString NO_DEVICE = "No device selected";

void MainWindow::populateDeviceCombos()
{
    m_blockSignals = true;

    m_comboIn->clear();
    m_comboOut->clear();

    for (const QString &s : m_midi->inputDevices())  m_comboIn->addItem(s);
    m_comboIn->addItem(NO_DEVICE);

    for (const QString &s : m_midi->outputDevices()) m_comboOut->addItem(s);
    m_comboOut->addItem(NO_DEVICE);

    m_comboIn->setCurrentIndex(m_comboIn->count() - 1);
    m_comboOut->setCurrentIndex(m_comboOut->count() - 1);

    m_blockSignals = false;
}

void MainWindow::onRefreshDevices()
{
    m_midi->refresh();
    populateDeviceCombos();
}

void MainWindow::onMidiInputChanged(int index)
{
    if (m_blockSignals) return;
    m_midi->closeInput();
    if (m_comboIn->itemText(index) != NO_DEVICE)
        m_midi->openInput(index);
}

void MainWindow::onMidiOutputChanged(int index)
{
    if (m_blockSignals) return;
    m_midi->closeOutput();
    if (m_comboOut->itemText(index) != NO_DEVICE) {
        m_midi->openOutput(index);
        sendAnchorPoints();
        sendVelocityRange();
        sendInputRange();
    }
}

// ──────────────────────────────────────────────
//  Parameter change handlers
// ──────────────────────────────────────────────
void MainWindow::onP1XChanged(double v)
{
    clearHistory();
    m_canvas->setP1((float)v, (float)m_spnP1Y->value());
    sendAnchorPoints();
}
void MainWindow::onP1YChanged(double v)
{
    clearHistory();
    m_canvas->setP1((float)m_spnP1X->value(), (float)v);
    sendAnchorPoints();
}
void MainWindow::onP2XChanged(double v)
{
    clearHistory();
    m_canvas->setP2((float)v, (float)m_spnP2Y->value());
    sendAnchorPoints();
}
void MainWindow::onP2YChanged(double v)
{
    clearHistory();
    m_canvas->setP2((float)m_spnP2X->value(), (float)v);
    sendAnchorPoints();
}
void MainWindow::onP1EnableChanged(bool en)
{
    clearHistory();
    m_canvas->setP1Enabled(en);
    sendAnchorPoints();
}
void MainWindow::onP2EnableChanged(bool en)
{
    clearHistory();
    m_canvas->setP2Enabled(en);
    sendAnchorPoints();
}
void MainWindow::onMinVelChanged(int v) { clearHistory(); m_minVelocity = v; sendVelocityRange(); }
void MainWindow::onMaxVelChanged(int v) { clearHistory(); m_maxVelocity = v; sendVelocityRange(); }
void MainWindow::onMinInputChanged(int v){ clearHistory(); m_minInput = v;    sendInputRange();    }
void MainWindow::onMaxInputChanged(int v){ clearHistory(); m_maxInput = v;    sendInputRange();    }

// Canvas drag → update spinboxes (block signal to avoid loop)
void MainWindow::onCanvasP1Changed(float x, float y)
{
    m_blockSignals = true;
    m_spnP1X->setValue(x);
    m_spnP1Y->setValue(y);
    m_blockSignals = false;
    sendAnchorPoints();
}
void MainWindow::onCanvasP2Changed(float x, float y)
{
    m_blockSignals = true;
    m_spnP2X->setValue(x);
    m_spnP2Y->setValue(y);
    m_blockSignals = false;
    sendAnchorPoints();
}

// ──────────────────────────────────────────────
//  MIDI note received → show on canvas
// ──────────────────────────────────────────────
float MainWindow::bezierCoord(float t, float p0, float p1c, float p2c, float p3) const
{
    float u = 1.0f - t;
    return u*u*u*p0 + 3*u*u*t*p1c + 3*u*t*t*p2c + t*t*t*p3;
}

void MainWindow::onNoteOn(int /*channel*/, int /*note*/, int velocity)
{
    int vel = velocity;
    vel = qMax(vel, m_minVelocity);
    vel = qMin(vel, m_maxVelocity);

    float t = (float)(vel - m_minVelocity) / (float)(m_maxVelocity - m_minVelocity);
    t = clampf(t, 0, 1);

    float p1x = (float)m_canvas->p1().x(), p1y = (float)m_canvas->p1().y();
    float p2x = (float)m_canvas->p2().x(), p2y = (float)m_canvas->p2().y();
    bool  e1  = m_canvas->p1Enabled(),      e2  = m_canvas->p2Enabled();

    float outX, outY;
    if (e1 && e2) {
        outX = bezierCoord(t, 0, p1x, p2x, 127);
        outY = bezierCoord(t, 0, p1y, p2y, 127);
    } else if (e1) {
        outX = bezierCoord(t, 0, p1x, p1x, 127);
        outY = bezierCoord(t, 0, p1y, p1y, 127);
    } else if (e2) {
        outX = bezierCoord(t, 0, p2x, p2x, 127);
        outY = bezierCoord(t, 0, p2y, p2y, 127);
    } else {
        outX = t * 127; outY = t * 127;
    }

    // velT = outX/127, outVal = outY
    m_canvas->addHistoryPoint(outX / 127.0f, outY);
    m_canvas->setCurrentPoint(outX / 127.0f, outY);
}

// ──────────────────────────────────────────────
//  SysEx senders (identical to C# version)
// ──────────────────────────────────────────────
void MainWindow::sendAnchorPoints()
{
    if (!m_midi->isOutputOpen()) return;
    quint8 p1x = (quint8)m_spnP1X->value(), p1y = (quint8)m_spnP1Y->value();
    quint8 p2x = (quint8)m_spnP2X->value(), p2y = (quint8)m_spnP2Y->value();
    quint8 e1  = m_chkP1->isChecked() ? 1 : 0;
    quint8 e2  = m_chkP2->isChecked() ? 1 : 0;

    QByteArray msg;
    msg.append((char)0xF0).append((char)0x11).append((char)0x12)
       .append((char)0x13).append((char)0x00)
       .append((char)p1x).append((char)p1y)
       .append((char)p2x).append((char)p2y)
       .append((char)e1).append((char)e2)
       .append((char)0xF7);
    m_midi->sendSysEx(msg);
}

void MainWindow::sendVelocityRange()
{
    if (!m_midi->isOutputOpen()) return;
    if (m_minVelocity > m_maxVelocity) return;
    QByteArray msg;
    msg.append((char)0xF0).append((char)0x11).append((char)0x12)
       .append((char)0x13).append((char)0x01)
       .append((char)(m_minVelocity & 0x7F))
       .append((char)(m_maxVelocity & 0x7F))
       .append((char)0xF7);
    m_midi->sendSysEx(msg);
}

void MainWindow::sendInputRange()
{
    if (!m_midi->isOutputOpen()) return;
    if (m_minInput > m_maxInput) return;
    char minLow  = (char)(m_minInput  & 0x7F);
    char minHigh = (char)((m_minInput  >> 7) & 0x7F);
    char maxLow  = (char)(m_maxInput  & 0x7F);
    char maxHigh = (char)((m_maxInput  >> 7) & 0x7F);
    QByteArray msg;
    msg.append((char)0xF0).append((char)0x11).append((char)0x12)
       .append((char)0x13).append((char)0x02)
       .append(minLow).append(minHigh)
       .append(maxLow).append(maxHigh)
       .append((char)0xF7);
    m_midi->sendSysEx(msg);
}

// ──────────────────────────────────────────────
//  History helper
// ──────────────────────────────────────────────
void MainWindow::clearHistory()
{
    m_canvas->clearHistory();
    m_canvas->clearCurrentPoint();
}

// ──────────────────────────────────────────────
//  File: Save / Load  (JSON – same fields as C#)
// ──────────────────────────────────────────────
void MainWindow::onSave()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save Curve Settings"), QString(),
        tr("JSON files (*.json);;All files (*.*)"));
    if (path.isEmpty()) return;

    QJsonObject obj;
    obj["P1X"]         = m_spnP1X->value();
    obj["P1Y"]         = m_spnP1Y->value();
    obj["P2X"]         = m_spnP2X->value();
    obj["P2Y"]         = m_spnP2Y->value();
    obj["P1Enabled"]   = m_chkP1->isChecked();
    obj["P2Enabled"]   = m_chkP2->isChecked();
    obj["MinVelocity"] = m_minVelocity;
    obj["MaxVelocity"] = m_maxVelocity;
    obj["MinInput"]    = m_minInput;
    obj["MaxInput"]    = m_maxInput;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot write file."));
        return;
    }
    f.write(QJsonDocument(obj).toJson());
}

void MainWindow::onLoad()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Load Curve Settings"), QString(),
        tr("JSON files (*.json);;All files (*.*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot read file."));
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) { QMessageBox::warning(this, tr("Error"), tr("Invalid JSON.")); return; }

    QJsonObject obj = doc.object();

    m_blockSignals = true;
    m_spnP1X->setValue(obj["P1X"].toDouble());
    m_spnP1Y->setValue(obj["P1Y"].toDouble());
    m_spnP2X->setValue(obj["P2X"].toDouble());
    m_spnP2Y->setValue(obj["P2Y"].toDouble());
    m_chkP1->setChecked(obj["P1Enabled"].toBool());
    m_chkP2->setChecked(obj["P2Enabled"].toBool());
    m_spnMinVel->setValue(obj["MinVelocity"].toInt());
    m_spnMaxVel->setValue(obj["MaxVelocity"].toInt());
    m_spnMinInput->setValue(obj["MinInput"].toInt());
    m_spnMaxInput->setValue(obj["MaxInput"].toInt());
    m_blockSignals = false;

    // Propagate to canvas
    m_canvas->setP1((float)m_spnP1X->value(), (float)m_spnP1Y->value());
    m_canvas->setP2((float)m_spnP2X->value(), (float)m_spnP2Y->value());
    m_canvas->setP1Enabled(m_chkP1->isChecked());
    m_canvas->setP2Enabled(m_chkP2->isChecked());

    sendAnchorPoints();
    sendVelocityRange();
    sendInputRange();
    clearHistory();
}
