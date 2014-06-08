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

#ifndef AUDIOCALLBACKS_H
#define AUDIOCALLBACKS_H

#include "raopcallbacks.h"

#include "audiooutput.h"

class AudioCallbacks : public RaopAudioHandler
{
    Q_OBJECT
public:
    explicit AudioCallbacks(QObject *parent = 0);

    virtual void *audioInit(int bits, int channels, int samplerate);
    virtual void audioSetVolume(void *session, float volume);
    virtual void audioProcess(void *session, const QByteArray &buffer);
    virtual void audioFlush(void *session);
    virtual void audioDestroy(void *session);


private:
    QList<AudioOutput*>  m_outputList;

signals:

public slots:

};

#endif // AUDIOCALLBACKS_H
