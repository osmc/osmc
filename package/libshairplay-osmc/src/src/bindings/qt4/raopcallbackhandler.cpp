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

#include "raopcallbackhandler.h"

RaopCallbackHandler::RaopCallbackHandler(QObject *parent) :
    QObject(parent)
{
}

void RaopCallbackHandler::init(RaopAudioHandler *callbacks)
{
    m_callbacks = callbacks;
}

void RaopCallbackHandler::audioInit(void *session, int bits, int channels, int samplerate)
{
    void **retval = (void**)session;
    if (m_callbacks) {
        *retval = m_callbacks->audioInit(bits, channels, samplerate);
    }
}

void RaopCallbackHandler::audioProcess(void *session, void *buffer, int buflen)
{
    if (m_callbacks) {
        m_callbacks->audioProcess(session, QByteArray((const char *)buffer, buflen));
    }
}

void RaopCallbackHandler::audioDestroy(void *session)
{
    if (m_callbacks) {
        m_callbacks->audioDestroy(session);
    }
}

void RaopCallbackHandler::audioFlush(void *session)
{
    if (m_callbacks) {
        m_callbacks->audioFlush(session);
    }
}

void RaopCallbackHandler::audioSetVolume(void *session, float volume)
{
    if (m_callbacks) {
        m_callbacks->audioSetVolume(session, volume);
    }
}

void RaopCallbackHandler::audioSetMetadata(void *session, void *buffer, int buflen)
{
    if (m_callbacks) {
        m_callbacks->audioSetMetadata(session, QByteArray((const char *)buffer, buflen));
    }
}

void RaopCallbackHandler::audioSetCoverart(void *session, void *buffer, int buflen)
{
    if (m_callbacks) {
        m_callbacks->audioSetCoverart(session, QByteArray((const char *)buffer, buflen));
    }
}

