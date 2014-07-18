/**
 * Copyright (C) 2012  Juho Vähä-Herttua
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef RAOPCALLBACKS_H
#define RAOPCALLBACKS_H

#include <QObject>

class RaopLogHandler : public QObject
{
    Q_OBJECT
public:
    explicit RaopLogHandler(QObject *parent = 0) : QObject(parent) {}

    virtual void logCallback(int level, const char *msg) = 0;
};

class RaopAudioHandler : public QObject
{
    Q_OBJECT
public:
    explicit RaopAudioHandler(QObject *parent = 0) : QObject(parent) {}

    virtual void *audioInit(int bits, int channels, int samplerate) = 0;
    virtual void audioProcess(void *session, const QByteArray & buffer) = 0;
    virtual void audioDestroy(void *session) = 0;

    virtual void audioFlush(void *session) { Q_UNUSED(session) }
    virtual void audioSetVolume(void *session, float volume) { Q_UNUSED(session) Q_UNUSED(volume) }
    virtual void audioSetMetadata(void *session, const QByteArray & buffer) { Q_UNUSED(session) Q_UNUSED(buffer) }
    virtual void audioSetCoverart(void *session, const QByteArray & buffer) { Q_UNUSED(session) Q_UNUSED(buffer) }
};

#endif // RAOPCALLBACKS_H
