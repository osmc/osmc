import connman
import dbus
import time

ETHERNET_PATH = '/net/connman/service/ethernet'

manager = connman.get_manager_interface()

def get_network_settings():
    for entry in manager.GetServices():
        path = entry[0]
        dbus_properties = entry[1]
        if path.startswith(ETHERNET_PATH):
            props = {'path' : path}
            # get IPv4 Data 
            ipv4_props = dbus_properties['IPv4']
            props['Method'] = str(ipv4_props['Method'])
            props['Address'] = str(ipv4_props['Address'])
            props['Netmask'] = str(ipv4_props['Netmask'])
            props['Gateway'] = str(ipv4_props['Gateway'])
            # Get  DNS Servers
            nameservers = dbus_properties['Nameservers']
            count=1
            for address in nameservers:
                props['DNS_'+str(count)] = str(address)
            # get state
            props['State'] = str(dbus_properties['State'])
            # get Interface name
            eth_props = dbus_properties['Ethernet']
            props['Interface'] = str(eth_props['Interface'])
            return props
    return None
        
        
def apply_network_changes(settings_dict):
    path = settings_dict['path']
    service = connman.get_service_interface(path)
    properties = service.GetProperties()
    ipv4_configuration = { 'Method': make_variant(settings_dict['Method']) }
    ipv4_configuration['Address'] = make_variant(settings_dict['Address'])
    ipv4_configuration['Netmask'] = make_variant(settings_dict['Netmask'])
    ipv4_configuration['Gateway'] = make_variant(settings_dict['Gateway'])
    service.SetProperty('IPv4.Configuration', ipv4_configuration)
    time.sleep(2)
    dns = []
    if 'DNS_1' in settings_dict:
        dns.append(settings_dict['DNS_1'])
    if 'DNS_2' in settings_dict:
        dns.append(settings_dict['DNS_2'])
    # duplicate SetProperty message works around connman dns forwarder bug
    service.SetProperty('Nameservers.Configuration', dbus.Array(dns, signature=dbus.Signature('s')))
    service.SetProperty('Nameservers.Configuration', dbus.Array(dns, signature=dbus.Signature('s')))


def make_variant(string):
    return dbus.String(string, variant_level=1)