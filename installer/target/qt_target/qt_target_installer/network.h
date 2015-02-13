/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef NETWORK_H
#define NETWORK_H
#include <QString>

class Network
{
public:
    Network();
    QString getIP() { return ip; }
    QString getMask() { return mask; }
    QString getDNS1() { return dns1; }
    QString getDNS2() { return dns2; }
    QString getGW() { return gw; }
    void setIP(QString ip) { this->ip = ip; }
    void setMask(QString mask) { this->mask = mask; }
    void setDNS1(QString dns1) { this->dns1 = dns1; }
    void setDNS2(QString dns2) { this->dns2 = dns2; }
    void setGW(QString gw) { this->gw = gw; }
    void setAuto() { this->dhcp = true; }
    void bringUp();
    bool isDefined() { return (! ip.isEmpty() && !mask.isEmpty() && ! dns1.isEmpty() & ! dns2.isEmpty() && ! gw.isEmpty()) ? true : false; }
private:
    QString ip;
    QString mask;
    QString dns1;
    QString dns2;
    QString gw;
    bool dhcp;
};

#endif // NETWORK_H
