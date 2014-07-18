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

#include "dnssdservice.h"

DnssdService::DnssdService(QObject *parent) :
    QObject(parent)
{
}

bool DnssdService::init()
{
    int error;
    m_dnssd = dnssd_init(&error);
    if (!m_dnssd) {
        return false;
    }
    return true;
}

DnssdService::~DnssdService()
{
    dnssd_destroy(m_dnssd);
}

void DnssdService::registerRaop(const QString & name, quint16 port, const QByteArray & hwaddr)
{
    dnssd_register_raop(m_dnssd, name.toUtf8().data(), port, hwaddr.data(), hwaddr.size(), 0);
}

void DnssdService::unregisterRaop()
{
    dnssd_unregister_raop(m_dnssd);
}

void DnssdService::registerAirplay(const QString &name, quint16 port, const QByteArray &hwaddr)
{
    dnssd_register_airplay(m_dnssd, name.toUtf8().data(), port, hwaddr.data(), hwaddr.size());
}

void DnssdService::unregisterAirplay()
{
    dnssd_unregister_airplay(m_dnssd);
}
