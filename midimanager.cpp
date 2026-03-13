#include "midimanager.h"
#include <QDebug>

#ifdef Q_OS_WIN
MidiManager *MidiManager::s_instance = nullptr;
#endif

MidiManager::MidiManager(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    s_instance = this;
#endif
    enumerateDevices();
}

MidiManager::~MidiManager()
{
    closeInput();
    closeOutput();
}

void MidiManager::enumerateDevices()
{
    m_inputNames.clear();
    m_outputNames.clear();

#ifdef Q_OS_WIN
    UINT numIn = midiInGetNumDevs();
    for (UINT i = 0; i < numIn; ++i) {
        MIDIINCAPS caps;
        if (midiInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
            m_inputNames << QString::fromWCharArray(caps.szPname);
    }

    UINT numOut = midiOutGetNumDevs();
    for (UINT i = 0; i < numOut; ++i) {
        MIDIOUTCAPS caps;
        if (midiOutGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
            m_outputNames << QString::fromWCharArray(caps.szPname);
    }
#else
    // On non-Windows platforms, leave empty or extend with ALSA/CoreMIDI
    Q_UNUSED(this);
#endif
}

void MidiManager::refresh()
{
    closeInput();
    closeOutput();
    enumerateDevices();
}

QStringList MidiManager::inputDevices() const  { return m_inputNames; }
QStringList MidiManager::outputDevices() const { return m_outputNames; }

#ifdef Q_OS_WIN
void CALLBACK MidiManager::midiInCallback(HMIDIIN /*hMidiIn*/,
                                           UINT wMsg,
                                           DWORD_PTR /*dwInstance*/,
                                           DWORD_PTR dwParam1,
                                           DWORD_PTR /*dwParam2*/  )
{
    if (wMsg == MIM_DATA && s_instance) {
        DWORD msg = (DWORD)dwParam1;
        quint8 status = msg & 0xFF;
        quint8 d1     = (msg >> 8)  & 0xFF;
        quint8 d2     = (msg >> 16) & 0xFF;

        // NoteOn with velocity > 0
        if ((status & 0xF0) == 0x90 && d2 > 0) {
            QMetaObject::invokeMethod(s_instance, "noteOn",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, status & 0x0F),
                                      Q_ARG(int, d1),
                                      Q_ARG(int, d2));
        }
    }
}
#endif

bool MidiManager::openInput(int deviceIndex)
{
    closeInput();
    if (deviceIndex < 0 || deviceIndex >= m_inputNames.size()) return false;

#ifdef Q_OS_WIN
    HMIDIIN h;
    MMRESULT res = midiInOpen(&h, (UINT)deviceIndex,
                               (DWORD_PTR)midiInCallback,
                               0, CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) return false;
    midiInStart(h);
    m_hMidiIn = h;
    m_inputOpen = true;
    return true;
#else
    Q_UNUSED(deviceIndex);
    return false;
#endif
}

void MidiManager::closeInput()
{
#ifdef Q_OS_WIN
    if (m_hMidiIn) {
        midiInStop((HMIDIIN)m_hMidiIn);
        midiInClose((HMIDIIN)m_hMidiIn);
        m_hMidiIn = nullptr;
    }
#endif
    m_inputOpen = false;
}

bool MidiManager::openOutput(int deviceIndex)
{
    closeOutput();
    if (deviceIndex < 0 || deviceIndex >= m_outputNames.size()) return false;

#ifdef Q_OS_WIN
    HMIDIOUT h;
    MMRESULT res = midiOutOpen(&h, (UINT)deviceIndex, 0, 0, CALLBACK_NULL);
    if (res != MMSYSERR_NOERROR) return false;
    m_hMidiOut = h;
    m_outputOpen = true;
    return true;
#else
    Q_UNUSED(deviceIndex);
    return false;
#endif
}

void MidiManager::closeOutput()
{
#ifdef Q_OS_WIN
    if (m_hMidiOut) {
        midiOutClose((HMIDIOUT)m_hMidiOut);
        m_hMidiOut = nullptr;
    }
#endif
    m_outputOpen = false;
}

void MidiManager::sendSysEx(const QByteArray &data)
{
#ifdef Q_OS_WIN
    if (!m_hMidiOut) return;
    MIDIHDR hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.lpData         = const_cast<char*>(data.constData());
    hdr.dwBufferLength = (DWORD)data.size();
    hdr.dwFlags        = 0;
    midiOutPrepareHeader((HMIDIOUT)m_hMidiOut, &hdr, sizeof(hdr));
    midiOutLongMsg((HMIDIOUT)m_hMidiOut, &hdr, sizeof(hdr));
    // Wait for completion (simple blocking approach)
    while (!(hdr.dwFlags & MHDR_DONE))
        Sleep(1);
    midiOutUnprepareHeader((HMIDIOUT)m_hMidiOut, &hdr, sizeof(hdr));
#else
    Q_UNUSED(data);
#endif
}
