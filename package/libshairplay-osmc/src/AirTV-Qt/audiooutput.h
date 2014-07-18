/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QObject>
#include <QIODevice>
#include <QByteArray>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QIODevice>

class AudioOutput : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = 0);
    bool init(int bits, int channels, int samplerate);
    bool setDevice(QAudioDeviceInfo deviceInfo);

    void start();
    void setVolume(float volume);
    void output(const QByteArray & data);
    void flush();
    void stop();

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 bytesAvailable() const;
    bool isSequential() const;

private:
    void reinit();

private:
    bool             m_initialized;
    QByteArray       m_buffer;
    QAudioFormat     m_format;
    QAudioDeviceInfo m_deviceInfo;
    QAudioOutput*    m_output;
    float            m_volume;

signals:

public slots:

private slots:
    void notified();
    void stateChanged(QAudio::State state);
};

#endif // AUDIOOUTPUT_H
