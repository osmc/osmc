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
    void setIP(QString ip) { ip = ip; }
    void setMask(QString mask) { mask = mask; }
    void setDNS1(QString dns1) { dns1 = dns1; }
    void setDNS2(QString dns2) { dns2 = dns2; }
    void setGW(QString gw) { gw = gw; }
    void setAuto() { dhcp = true; }
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
