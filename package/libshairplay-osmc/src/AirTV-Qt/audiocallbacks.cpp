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

#include "audiocallbacks.h"

AudioCallbacks::AudioCallbacks(QObject *parent) :
    RaopAudioHandler(parent)
{
}

void * AudioCallbacks::audioInit(int bits, int channels, int samplerate)
{
    AudioOutput *audioOutput = new AudioOutput(0);
    audioOutput->init(bits, channels, samplerate);
    audioOutput->start();
    m_outputList.append(audioOutput);
    return audioOutput;
}

void AudioCallbacks::audioProcess(void *session, const QByteArray & buffer)
{
    AudioOutput *audioOutput = (AudioOutput*)session;
    audioOutput->output(buffer);
}

void AudioCallbacks::audioDestroy(void *session)
{
    AudioOutput *audioOutput = (AudioOutput*)session;
    m_outputList.removeAll(audioOutput);

    audioOutput->stop();
    delete audioOutput;
}


void AudioCallbacks::audioFlush(void *session)
{
    AudioOutput *audioOutput = (AudioOutput*)session;
    audioOutput->flush();
}

void AudioCallbacks::audioSetVolume(void *session, float volume)
{
    AudioOutput *audioOutput = (AudioOutput*)session;
    audioOutput->setVolume(volume);
}

