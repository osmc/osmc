import connman
import dbus
import time
import subprocess
import re
import sys
import os
import os.path
import requests
import systemd

WIRELESS_AGENT = '/usr/bin/preseed-agent'

ETHERNET_PATH = '/net/connman/service/ethernet'
WIFI_PATH = '/net/connman/service/wifi'

# this is where we read NFS network info from this is the current running config
RUNNING_NETWORK_DETAILS_FILE = '/proc/cmdline'
# but we want to update here - this gets copied to /proc/ as part of boot
UPDATE_NETWORK_DETAILS_FILE = '/boot/cmdline.txt'
PREESEED_TEMP_LOCATION = '/tmp/preseed.tmp'
PREESEED_LOCATION = '/boot/preseed.cfg'

WAIT_FOR_NETWORK_SERVICE = 'connman-wait-for-network.service'

manager = connman.get_manager_interface()


def is_ethernet_enabled():
    return connman.is_technology_enabled('ethernet') or not get_nfs_ip_cmdline_value() == None


def toggle_ethernet_state(state):
    connman.toggle_technology_state('ethernet', state)


def get_ethernet_settings():
    for entry in manager.GetServices():
        eth_settings = None
        path = entry[0]
        dbus_properties = entry[1]
        if path.startswith(ETHERNET_PATH):
            
            settings = extract_network_properties(dbus_properties)
            if settings:
                eth_settings = {'path': path }
                eth_settings.update(settings)
            return eth_settings

    # if we are here we have not detected a ethernet connection check for NFS
    ip_value = get_nfs_ip_cmdline_value()
    if ip_value:
        if ip_value == 'dhcp':
            nfs_settings = get_non_connman_connection_details()
            nfs_settings['Method'] = 'nfs_dhcp'
        else:
            nfs_settings = split_nfs_static_cmdlline(ip_value)
            nfs_settings['Method'] = 'nfs_manual'

        nfs_settings['Interface'] = 'eth0 (NFS)'
        nfs_settings['State'] = 'online'
        nfs_settings['path'] = 'nfs'
        if not check_MS_NCSI_response():
            nfs_settings['State'] = 'ready'
        return nfs_settings

    return None


def get_nfs_ip_cmdline_value():
    ip_value = None
    cmdline_data = open(RUNNING_NETWORK_DETAILS_FILE, 'r').read()
    nfs_install = False
    for cmdline_value in cmdline_data.split(' '):
        if 'root=/dev/nfs' in cmdline_value:
            nfs_install = True
        # grab the ip= value from cmdline
        if cmdline_value.startswith('ip='):
            ip_value = cmdline_value[3:]
    if nfs_install:
        return ip_value
    return None


def extract_network_properties(dbus_properties):
    settings = None
    # get IPv4 Data
    ipv4_props = dbus_properties['IPv4']
    if 'Address' in ipv4_props:
        settings = {}
        settings['Method'] = str(ipv4_props['Method'])
        settings['Address'] = str(ipv4_props['Address'])
        settings['Netmask'] = str(ipv4_props['Netmask'])
        if 'Gateway' in ipv4_props:
            settings['Gateway'] = str(ipv4_props['Gateway'])
        # Get  DNS Servers
        nameservers = dbus_properties['Nameservers']
        count = 1
        for nameserver in nameservers:
            settings['DNS_' + str(count)] = str(nameserver)
            count += 1
        # get state
        settings['State'] = str(dbus_properties['State'])
        # get Interface name
        eth_props = dbus_properties['Ethernet']
        settings['Interface'] = str(eth_props['Interface'])
    return settings


def apply_network_changes(settings_dict):
    if settings_dict['Method'] in ['manual', 'dhcp']:  # non NFS setup
        path = settings_dict['path']
        service = connman.get_service_interface(path)
        ipv4_configuration = {'Method': make_variant(settings_dict['Method']),
                              'Address': make_variant(settings_dict['Address']),
                              'Netmask': make_variant(settings_dict['Netmask'])}
        if 'Gateway' in settings_dict:
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
    elif settings_dict['Method'].startswith('nfs_'):
        ip_value = None
        if settings_dict['Method'] == 'nfs_dhcp':
            ip_value = 'dhcp'
        if settings_dict['Method'] == 'nfs_manual':
            ip_value = create_cmdline_nfs_manual_string(settings_dict)
        update_cmdline_file(UPDATE_NETWORK_DETAILS_FILE, 'ip', ip_value)


def make_variant(string):
    return dbus.String(string, variant_level=1)


def update_cmdline_file(file_path, key, value):
    cmdline_file = open(file_path, 'r')
    cmdline_data = cmdline_file.read()
    cmdline_file.close()
    cmdline_values = []
    for cmdline_value in cmdline_data.split(' '):
        if cmdline_value.startswith(key + '='):
            cmdline_values.extend([key + '=' + value])
        else:
            cmdline_values.extend([cmdline_value])
    updated_cmdline = ' '.join(cmdline_values)  # join the list with a space
    cmdline_file = open('/tmp/cmdline.txt', 'w')
    cmdline_file.write(updated_cmdline)
    cmdline_file.close()
    subprocess.call(['sudo', 'mv', '/tmp/cmdline.txt', file_path])


'''
  | Address   || gateway   | netmask     |    |    |   | DNS_1 | DNS_2
  192.168.1.20::192.168.1.1:255.255.255.0:osmc:eth0:off:8.8.8.8:8.8.4.4
'''


def split_nfs_static_cmdlline(value):
    connection_details = value.split(':')
    nfs_static = {'Address': connection_details[0], 'Gateway': connection_details[2], 'Netmask': connection_details[3]}
    if len(connection_details) > 6:
        nfs_static['DNS_1'] = connection_details[7]
    if len(connection_details) > 7:
        nfs_static['DNS_2'] = connection_details[8]
    return nfs_static


def create_cmdline_nfs_manual_string(settings_dict):
    cmd_string = settings_dict['Address'] + '::' + settings_dict['Gateway'] + ':' + settings_dict['Netmask']
    cmd_string += ':osmc:eth0:off'
    if 'DNS_1' in settings_dict:
        cmd_string = cmd_string + ':' + settings_dict['DNS_1']
    if 'DNS_2' in settings_dict:
        cmd_string = cmd_string + ':' + settings_dict['DNS_2']
    return cmd_string


def get_non_connman_connection_details():
    device = 'eth0'
    # execute ifconfig and capture the output
    proc = subprocess.Popen(['/sbin/ifconfig', device], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (ifconfig_data, stderr) = proc.communicate()
    proc.wait()
    # parse ifconfig using re
    data = re.findall(r'^(\S+).*?inet addr:(\S+).*?Mask:(\S+)', ifconfig_data, re.S | re.M)
    (name, address, netmask) = data[0]
    device_settings = {'Address': address, 'Netmask': netmask}
    # parse resolve.conf for DNS
    resolve_conf_file = open('/etc/resolv.conf', 'r')
    resolve_conf_data = resolve_conf_file.read()
    resolve_conf_file.close()
    count = 1
    for line in resolve_conf_data.split('\n'):
        if line.startswith('nameserver'):
            ip = line.replace('nameserver ', '').strip()
            if len(ip.split('.')) == 4:  # ipV4 address
                device_settings['DNS_' + str(count)] = str(ip)
    # get default gateway
    proc = subprocess.Popen(['/sbin/route', '-n'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (route_data, stderr) = proc.communicate()
    proc.wait()
    for line in route_data.split('\n'):
        if line.startswith('0.0.0.0'):
            split = [value.strip() for value in line.split('    ')]
            device_settings['Gateway'] = split[3]

    return device_settings


def is_wifi_available():
    return connman.is_technology_available('wifi')


def is_wifi_enabled():
    return connman.is_technology_enabled('wifi')


def toggle_wifi_state(state):
    connman.toggle_technology_state('wifi', state)


def scan_wifi():
    wifi = connman.get_technology_interface('wifi')
    try:
        wifi.Scan()
    except dbus.DBusException:
        # technology.Scan() will sometimes time out if connman is "busy". Catch exception and retry.
        try:
            time.sleep(1)
            wifi.Scan()
        except dbus.DBusException:
            pass


def get_wifi_networks():
    wifis = {}
    for entry in manager.GetServices():
        path = entry[0]
        dbus_properties = entry[1]
        if path.startswith(WIFI_PATH) and 'hidden' not in path:
            wifi_settings = {'path': str(path), 'SSID': dbus_properties['Name'].encode('UTF-8'),
                     'Strength': int(dbus_properties['Strength']), 'State' : str(dbus_properties['State'])}
            security_props = dbus_properties['Security']
            if len(security_props) > 0:
                wifi_settings['Security'] = str(security_props[0])
                
            if not str(dbus_properties['State']) == 'idle' and not str(dbus_properties['State']) == 'failure':
                settings = extract_network_properties(dbus_properties)
                if settings:
                    wifi_settings.update(settings)
            wifis[wifi_settings['SSID']] = wifi_settings
    return wifis


def wifi_connect(path, password=None):
    connected = False
    if password:
        print 'Starting Agent'
        keyfile = open('/tmp/key', 'w')
        keyfile.write(password)
        keyfile.close()
        process = subprocess.Popen([sys.executable, WIRELESS_AGENT, 'keyfile'])
	count = 0
	while not os.path.exists('/tmp/agent_started'):
	    count+=1
	    time.sleep(1)
	    if count > 10:
                print 'Agent had not started after 10 seconds'
		break 
	if os.path.exists('/tmp/agent_started'):
	    os.remove('/tmp/agent_started')
    print ('Attempting connection to ' + path )
    service = connman.get_service_interface(path)
    try:
        service.Connect(timeout=15000)
        connected = True
    except dbus.DBusException, e:
        print ('DBusException connecting ' + str(e))
        connected = False
    print (connected)
    if password:
        process.kill()
        os.remove('/tmp/key')
    return connected


def wifi_disconnect(path):
    service = connman.get_service_interface(path)
    try:
        service.Disconnect()
    except dbus.DBusException, e:
        print ('DBusException disconnecting')
        connected = False


def wifi_remove(path):
    service = connman.get_service_interface(path)
    try:
        service.Remove()
    except dbus.DBusException, e:
        print ('DBusException removing')
        connected = False


def get_connected_wifi():
    for ssid, value in get_wifi_networks().iteritems():
        if value['State'] in ('online', 'ready'):
            return value
    return {}

def has_internet_connection():
    ethernet_settings = get_ethernet_settings()
    if ethernet_settings:
        if 'Method' in ethernet_settings:
            if ethernet_settings['Method'].startswith('nfs_'):
                return check_MS_NCSI_response()
            elif 'State' in ethernet_settings and ethernet_settings['State'] == 'online':
                return True
    wifi_networks = get_wifi_networks()
    for ssid in wifi_networks.keys():
        info = wifi_networks[ssid]
        if info['State'] == 'online':
            return True
    return False


def check_MS_NCSI_response():
    response =  requests.get('http://www.msftncsi.com/ncsi.txt')
    if response.status_code == 200 and response.text == 'Microsoft NCSI':
        return True
    return False


def update_preseed_file(settings_dict):
    if os.path.isfile(PREESEED_LOCATION):
        filtered_settings_dict = settings_dict.copy()
        if 'Password' in filtered_settings_dict:
            filtered_settings_dict['Password'] = '********'
        print filtered_settings_dict
        wireless_password_line = None
        # reed the current data in
        preseed_file = open(PREESEED_LOCATION, 'r')
        preseed_data = preseed_file.read()
        preseed_file.close()
        # re-open file for writing
        preseed_file = open(PREESEED_TEMP_LOCATION, 'w')
        # copy everything except the network entries to a temp file
        for entry in preseed_data.split("\n"):
            if not entry.startswith('d-i network/') and len(entry.strip()) > 0:
                # write this entry back to the file as is
                preseed_file.write(entry + "\n")
            if entry.startswith('d-i network/wlan_key'):
                wireless_password_line =  entry;

        # write the required network options to the temp file
        if settings_dict:
            if 'eth' in settings_dict['path'] or settings_dict['path'] == 'nfs':
                preseed_file.write('d-i network/interface string eth\n')
            elif 'wifi' in settings_dict['path']:
                preseed_file.write('d-i network/interface string wlan\n')
                preseed_file.write('d-i network/ssid string ' + settings_dict['SSID'] + '\n')
                if 'Security' in settings_dict:
                    if settings_dict['Security'] in ('wpa', 'wpa2', 'pak'):
                        preseed_file.write('d-i network/wlan_keytype string 1\n')
                    elif settings_dict['Security'] in ('wep'):
                        preseed_file.write('d-i network/wlan_keytype string 2\n')
                    if wireless_password_line: # preserve the password as we may not always get it from the GUI
                        preseed_file.write(wireless_password_line + '\n')
                    else:
                        preseed_file.write('d-i network/wlan_key string ' + settings_dict['Password'] + '\n')
                else:
                    preseed_file.write('d-i network/wlan_keytype string 0\n')

            if 'dhcp' in settings_dict['Method']:
                preseed_file.write('d-i network/auto boolean true\n')
            if 'manual' in settings_dict['Method']:
                preseed_file.write('d-i network/auto boolean false\n')
                preseed_file.write('d-i network/ip string ' + settings_dict['Address'] + '\n')
                preseed_file.write('d-i network/mask string ' + settings_dict['Netmask'] + '\n')
                if 'Gateway' in settings_dict:
                    preseed_file.write('d-i network/gw string ' + settings_dict['Gateway'] + '\n')
                if 'DNS_1' in settings_dict:
                    preseed_file.write('d-i network/dns1 string ' + settings_dict['DNS_1'] + '\n')
                if 'DNS_2' in settings_dict:
                    preseed_file.write('d-i network/dns2 string ' + settings_dict['DNS_2'] + '\n')
        else: # empty dict passed - we have just disconnected from wifi - create auto wired as a fell back
            preseed_file.write('d-i network/interface string eth\n')
            preseed_file.write('d-i network/auto boolean true\n')
        preseed_file.close()
        subprocess.call(['sudo', 'mv', PREESEED_TEMP_LOCATION, PREESEED_LOCATION])


def parse_preseed():
    network_settings = None
    if os.path.isfile(PREESEED_LOCATION):
        preseed_file = open(PREESEED_LOCATION, 'r')
        preseed_data = preseed_file.read()
        preseed_file.close()
        preseed_info = {}
        for entry in preseed_data.split("\n"):
            if entry.startswith('d-i network/') and len(entry.strip()) > 0:
                string = entry.replace('d-i network/', '')
                key = string[:string.find(' ')]
                string = string[string.find(' ')+1:]
                type = string[:string.find(' ')]
                value = string[string.find(' ')+1:]
                preseed_info[key] = value
        # convert preseed format into the same format we are using in the rest of the code
        network_settings = {'Interface': preseed_info['interface']}
        if 'wlan' in preseed_info['interface']:
            network_settings['SSID'] = preseed_info['ssid']
            if 'wlan_key' in preseed_info:
                network_settings['Password'] = preseed_info['wlan_key']
        if 'true' in preseed_info['auto']:
            network_settings['Method'] = 'dhcp'
        else:
            network_settings['Method'] = 'Manual'
            network_settings['Address'] = preseed_info['ip']
            network_settings['Netmask'] = preseed_info['mask']
            if 'gw' in preseed_info:
                network_settings['Gateway'] = preseed_info['gw']
            if 'dns1' in preseed_info:
                network_settings['DNS_1'] = preseed_info['dns1']
            if 'dns2' in preseed_info:
                network_settings['DNS_1'] = preseed_info['dns1']
    return network_settings


def is_connman_wait_for_network_enabled():
    return systemd.is_service_enabled(WAIT_FOR_NETWORK_SERVICE);


def toggle_wait_for_network(state):
    if state:
        systemd.update_service(WAIT_FOR_NETWORK_SERVICE, 'enable')
    else:
        systemd.update_service(WAIT_FOR_NETWORK_SERVICE, 'disable')
