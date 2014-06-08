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

#include "audiooutput.h"

#include <QDebug>
#include <QtEndian>
#include <math.h>

#define BUFFER_SIZE (64*1024)

AudioOutput::AudioOutput(QObject *parent) :
    QIODevice(parent),
    m_initialized(false),
    m_output(0),
    m_volume(0.0f)
{
}

bool AudioOutput::init(int bits, int channels, int samplerate)
{
    if (m_initialized) {
        return false;
    }
    if (bits != 16) {
        return false;
    }

    m_format.setSampleSize(bits);
    m_format.setChannels(channels);
    m_format.setFrequency(samplerate);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    m_initialized = setDevice(QAudioDeviceInfo::defaultOutputDevice());
    return m_initialized;
}

bool AudioOutput::setDevice(QAudioDeviceInfo deviceInfo)
{
    if (!deviceInfo.isFormatSupported(m_format)) {
        qDebug() << "Format not supported!";
        return false;
    }
    m_deviceInfo = deviceInfo;
    this->reinit();
    return true;
}

void AudioOutput::reinit()
{
    bool running = false;
    if (m_output && m_output->state() != QAudio::StoppedState) {
        running = true;
    }
    this->stop();

    // Reinitialize audio output
    delete m_output;
    m_output = 0;
    m_output = new QAudioOutput(m_deviceInfo, m_format, this);

    // Set constant values to new audio output
    connect(m_output, SIGNAL(notify()), SLOT(notified()));
    connect(m_output, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));
    if (running) {
        this->start();
    }
}

void AudioOutput::start()
{
    if (m_output == 0 || m_output->state() != QAudio::StoppedState) {
        return;
    }
    this->open(QIODevice::ReadOnly);
    m_buffer.clear();
    m_output->start(this);
    m_output->suspend();
}

void AudioOutput::setVolume(float volume)
{
    m_volume = volume;
}

void AudioOutput::output(const QByteArray & data)
{
    if (m_output && m_output->state() != QAudio::StoppedState) {
        // Append input data to the end of buffer
        m_buffer.append(data);

        // Check if our buffer has grown too large
        if (m_buffer.length() > 2*BUFFER_SIZE) {
            // There could be a better way to handle this
            this->flush();
        }

        // If audio is suspended and buffer is full, resume
        if (m_output->state() == QAudio::SuspendedState) {
            if (m_buffer.length() >= BUFFER_SIZE) {
                qDebug() << "Resuming...";
                m_output->resume();
            }
        }
    }
}

void AudioOutput::flush()
{
    // Flushing buffers is a bit tricky...
    // Don't modify this unless you're sure
    this->stop();
    m_output->reset();
    this->start();
}

void AudioOutput::stop()
{
    if (m_output && m_output->state() != QAudio::StoppedState) {
        // Stop audio output
        m_output->stop();
        m_buffer.clear();
        this->close();
    }
}

static void apply_s16le_volume(float volume, uchar *data, int datalen)
{
    int samples = datalen/2;
    float mult = pow(10.0,0.05*volume);

    for (int i=0; i<samples; i++) {
        qint16 val = qFromLittleEndian<qint16>(data+i*2)*mult;
        qToLittleEndian<qint16>(val, data+i*2);
    }
}

qint64 AudioOutput::readData(char *data, qint64 maxlen)
{
    // Calculate output length, always full samples
    int outlen = qMin(m_buffer.length(), (int)maxlen);
    if (outlen%2 != 0) {
        outlen += 1;
    }

    memcpy(data, m_buffer.data(), outlen);
    apply_s16le_volume(m_volume, (uchar *)data, outlen);
    m_buffer.remove(0, outlen);
    return outlen;
}

qint64 AudioOutput::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 AudioOutput::bytesAvailable() const
{
    return m_buffer.length() + QIODevice::bytesAvailable();
}

bool AudioOutput::isSequential() const
{
    return true;
}

void AudioOutput::notified()
{
}

void AudioOutput::stateChanged(QAudio::State state)
{
    // Start buffering again in case of underrun...
    // Required on Windows, otherwise it stalls idle
    if (state == QAudio::IdleState && m_output->error() == QAudio::UnderrunError) {
        // This check is required, because Mac OS X underruns often
        if (m_buffer.length() < BUFFER_SIZE) {
            m_output->suspend();
        }
    }
    qWarning() << "state = " << state;
}
