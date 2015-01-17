/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef NETWORKSETTINGS_H
#define NETWORKSETTINGS_H
#include <QString>

class NetworkSettings
{
public:
    NetworkSettings();
    void setDHCP(bool useDHCP) { this->useDHCP = useDHCP; }
    void setWireless(bool useWireless) { this->useWireless = useWireless; }
    void setWirelessSSID(QString ssid) { this->wirelessSSID = ssid; }
    void setWirelessKeyType(int key_type) { this->wirelessKeyType = key_type; }
    void setWirelessKeyValue(QString key_val) { this->wirelessKeyValue = key_val; }
    void setIP(QString ip) { this->ip = ip; }
    void setMask(QString mask) { this->mask = mask; }
    void setGW(QString gw) { this->gw = gw; }
    void setDNS1(QString dns1) { this->dns1 = dns1; }
    void setDNS2(QString dns2) { this->dns2 = dns2; }
    bool hasDHCP() { return this->useDHCP; }
    bool hasWireless() { return this->useWireless; }
    QString getWirelessSSID() { return this->wirelessSSID; }
    int getWirelessKeyType() { return this->wirelessKeyType; }
    QString getWirelessKeyValue() { return this->wirelessKeyValue; }
    QString getIP() { return this->ip; }
    QString getMask() { return this->mask; }
    QString getGW() { return this->gw; }
    QString getDNS1() { return this->dns1; }
    QString getDNS2() { return this->dns2; }
private:
    bool useDHCP;
    bool useWireless;
    QString wirelessSSID;
    int wirelessKeyType;
    QString wirelessKeyValue;
    QString ip;
    QString mask;
    QString gw;
    QString dns1;
    QString dns2;
};

#endif // NETWORKSETTINGS_H
