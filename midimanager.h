#ifndef MIDIMANAGER_H
#define MIDIMANAGER_H

#include <QObject>
#include <QStringList>
#include <QVector>

// Must include windows.h BEFORE declaring CALLBACK in the class
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <mmsystem.h>
#endif

class MidiManager : public QObject
{
    Q_OBJECT
public:
    explicit MidiManager(QObject *parent = nullptr);
    ~MidiManager();

    // Device enumeration
    QStringList inputDevices() const;
    QStringList outputDevices() const;
    void refresh();

    // Open/close
    bool openInput(int deviceIndex);
    void closeInput();
    bool openOutput(int deviceIndex);
    void closeOutput();
    bool isInputOpen()  const { return m_inputOpen;  }
    bool isOutputOpen() const { return m_outputOpen; }

    // Send raw SysEx
    void sendSysEx(const QByteArray &data);

signals:
    void noteOn(int channel, int note, int velocity);

private:
    QStringList m_inputNames;
    QStringList m_outputNames;
    bool m_inputOpen  = false;
    bool m_outputOpen = false;

#ifdef Q_OS_WIN
    HMIDIIN  m_hMidiIn  = nullptr;
    HMIDIOUT m_hMidiOut = nullptr;
    static void CALLBACK midiInCallback(HMIDIIN hMidiIn, UINT wMsg,
                                        DWORD_PTR dwInstance,
                                        DWORD_PTR dwParam1,
                                        DWORD_PTR dwParam2);
    static MidiManager *s_instance;
#endif

    void enumerateDevices();
};

#endif // MIDIMANAGER_H
