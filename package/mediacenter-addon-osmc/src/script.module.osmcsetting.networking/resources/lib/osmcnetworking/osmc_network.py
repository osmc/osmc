# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.networking

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import os.path
import re
import socket
import subprocess
import sys
import time
from io import open

import requests

import dbus

from . import connman
from . import osmc_systemd

PY2 = sys.version_info.major == 2

WIRELESS_AGENT = 'osmc_wireless_agent.py'

ETHERNET_PATH = '/net/connman/service/ethernet'
WIFI_PATH = '/net/connman/service/wifi'

# this is where we read NFS network info from this is the current running config
RUNNING_NETWORK_DETAILS_FILE = '/proc/cmdline'
# but we want to update here - this gets copied to /proc/ as part of boot
UPDATE_NETWORK_DETAILS_FILE = '/boot/cmdline.txt'
PREESEED_TEMP_LOCATION = '/tmp/preseed.tmp'
PREESEED_LOCATION = '/boot/preseed.cfg'

WAIT_FOR_NETWORK_SERVICE = 'connman-wait-for-network.service'


def is_ethernet_enabled():
    return connman.is_technology_enabled('ethernet') or get_nfs_ip_cmdline_value() is not None


def toggle_ethernet_state(state):
    connman.toggle_technology_state('ethernet', state)


def get_ethernet_settings():
    manager = connman.get_manager_interface()
    for entry in manager.GetServices():
        eth_settings = None
        path = str(entry[0])
        dbus_properties = entry[1]
        if path.startswith(ETHERNET_PATH):
            settings = extract_network_properties(dbus_properties)
            if settings:
                eth_settings = {
                    'path': path
                }
                eth_settings.update(settings)
            return eth_settings

    # if we are here we have not detected a ethernet connection check for NFS
    ip_value = get_nfs_ip_cmdline_value()
    if ip_value:
        if ip_value == 'dhcp':
            nfs_settings = get_non_connman_connection_details()
            protocol = 'IPV4'
            if 'IPV6' in nfs_settings:
                protocol = 'IPV6'
            nfs_settings[protocol]['Method'] = 'nfs_dhcp'
        else:
            nfs_settings = split_nfs_static_cmdline(ip_value)
            protocol = 'IPV4'
            if 'IPV6' in nfs_settings:
                protocol = 'IPV6'
            nfs_settings[protocol]['Method'] = 'nfs_manual'

        nfs_settings['Interface'] = 'eth0 (NFS)'
        nfs_settings['State'] = 'online'
        nfs_settings['path'] = 'nfs'
        if not check_ms_ncsi_response():
            nfs_settings['State'] = 'ready'
        return nfs_settings

    return None


def get_nfs_ip_cmdline_value():
    ip_value = None
    nfs_install = False

    with open(RUNNING_NETWORK_DETAILS_FILE, 'r', encoding='utf-8') as cmdline:
        cmdline_data = cmdline.read()

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
    # get IPv4 Data
    ipv4_settings = {}
    ipv4_props = dbus_properties['IPv4']
    for key in {'Method', 'Address', 'Netmask', 'Gateway'}:
        if key in ipv4_props:
            ipv4_settings[key] = str(ipv4_props[key])
        else:
            ipv4_settings[key] = None

    # get IPv6 Data
    ipv6_settings = {}
    ipv6_props = dbus_properties['IPv6']
    for key in {'Method', 'Address', 'PrefixLength', 'Gateway', 'Privacy'}:
        if key in ipv6_props:
            ipv6_settings[key] = str(ipv6_props[key])
        else:
            ipv6_settings[key] = None

    dns_settings = {}
    # Get  DNS Servers
    name_servers = dbus_properties['Nameservers']
    count = 1
    for name_server in name_servers:
        dns_settings['DNS_' + str(count)] = str(name_server)
        count += 1

    for count in range(1, 3):
        if 'DNS_' + str(count) not in dns_settings:
            dns_settings['DNS_' + str(count)] = None

    eth_props = dbus_properties['Ethernet']
    settings = {
        'IPV4': ipv4_settings,
        'Nameservers': dns_settings,
        'State': str(dbus_properties['State']),
        'Interface': str(eth_props['Interface'])
    }

    return settings


def apply_network_changes(settings_dict, internet_protocol):
    if settings_dict[internet_protocol]['Method'] in ['manual', 'dhcp']:  # non NFS setup
        path = settings_dict['path']
        service = connman.get_service_interface(path)

        ipv4_configuration = {
            'Method': make_variant(settings_dict[internet_protocol]['Method']),
            'Address': make_variant(settings_dict[internet_protocol]['Address']),
            'Netmask': make_variant(settings_dict[internet_protocol]['Netmask'])
        }

        if settings_dict[internet_protocol]['Gateway']:
            ipv4_configuration['Gateway'] = \
                make_variant(settings_dict[internet_protocol]['Gateway'])

        service.SetProperty('IPv4.Configuration', ipv4_configuration)
        time.sleep(2)
        if settings_dict[internet_protocol]['Method'] == 'dhcp':
            dns = ['', '']

        else:
            namesevers = settings_dict['Nameservers']
            dns = []
            if 'DNS_1' in namesevers and namesevers['DNS_1']:
                dns.append(namesevers['DNS_1'])

            if 'DNS_2' in namesevers and namesevers['DNS_2']:
                dns.append(namesevers['DNS_2'])

        # duplicate SetProperty message works around connman dns forwarder bug
        service.SetProperty('Nameservers.Configuration',
                            dbus.Array(dns, signature=dbus.Signature('s')))
        service.SetProperty('Nameservers.Configuration',
                            dbus.Array(dns, signature=dbus.Signature('s')))

    elif settings_dict[internet_protocol]['Method'].startswith('nfs_'):
        ip_value = None
        if settings_dict[internet_protocol]['Method'] == 'nfs_dhcp':
            ip_value = 'dhcp'

        if settings_dict[internet_protocol]['Method'] == 'nfs_manual':
            ip_value = create_cmdline_nfs_manual_string(settings_dict, internet_protocol)

        update_cmdline_file(UPDATE_NETWORK_DETAILS_FILE, 'ip', ip_value)


def make_variant(string):
    return dbus.String(string, variant_level=1)


def update_cmdline_file(file_path, key, value):
    with open(file_path, 'r', encoding='utf-8') as cmdline_file:
        cmdline_data = cmdline_file.read()

    cmdline_values = []
    for cmdline_value in cmdline_data.split(' '):
        if cmdline_value.startswith(key + '='):
            cmdline_values.extend([key + '=' + value])

        else:
            cmdline_values.extend([cmdline_value])

    updated_cmdline = ' '.join(cmdline_values)  # join the list with a space
    with open('/tmp/cmdline.txt', 'w', encoding='utf-8') as cmdline_file:
        if PY2 and not isinstance(updated_cmdline, unicode):
            updated_cmdline = updated_cmdline.decode('utf-8')
        cmdline_file.write(updated_cmdline)

    subprocess.call(['sudo', 'mv', '/tmp/cmdline.txt', file_path])


'''
  | Address   || gateway   | netmask     |    |    |   | DNS_1 | DNS_2
  192.168.1.20::192.168.1.1:255.255.255.0:osmc:eth0:off:8.8.8.8:8.8.4.4
'''


def split_nfs_static_cmdline(value):
    nfs_static = None
    connection_details = value.split(':')

    ip_settings = {
        'Address': connection_details[0],
        'Gateway': connection_details[2],
        'Netmask': connection_details[3]
    }

    name_servers_settings = {}
    if len(connection_details) > 7:
        name_servers_settings['DNS_1'] = connection_details[7]

    else:
        name_servers_settings['DNS_1'] = None

    if len(connection_details) > 8:
        name_servers_settings['DNS_2'] = connection_details[8]

    else:
        name_servers_settings['DNS_2'] = None

    if is_valid_ipv6_address(ip_settings['Address']):
        nfs_static = {
            'IPV6': ip_settings,
            'Nameservers': name_servers_settings
        }

    if is_valid_ipv4_address(ip_settings['Address']):
        nfs_static = {
            'IPV4': ip_settings,
            'Nameservers': name_servers_settings
        }

    return nfs_static


def create_cmdline_nfs_manual_string(settings_dict, internet_protocol):
    cmd_string = settings_dict[internet_protocol]['Address'] + \
                 '::' + settings_dict[internet_protocol]['Gateway'] + ':' + \
                 settings_dict[internet_protocol]['Netmask']
    cmd_string += ':osmc:eth0:off'

    for num in range(1, 3):
        count = str(num)
        if ('DNS_' + count in settings_dict['Nameservers'] and
                settings_dict['Nameservers']['DNS_' + count]):
            cmd_string = cmd_string + ':' + settings_dict['Nameservers']['DNS_' + count]

    return cmd_string


def get_non_connman_connection_details():
    device = 'eth0'
    # execute ifconfig and capture the output
    proc = subprocess.Popen(['/sbin/ifconfig', device],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (ifconfig_data, stderr) = proc.communicate()
    proc.wait()
    # parse ifconfig using re
    data = re.findall(r'^(\S+).*?inet addr:(\S+).*?Mask:(\S+)', ifconfig_data, re.S | re.M)
    (name, address, netmask) = data[0]

    device_settings = {
        'IPV4': {
            'Address': address,
            'Netmask': netmask
        }
    }

    # parse resolve.conf for DNS
    with open('/etc/resolv.conf', 'r', encoding='utf-8') as resolve_conf_file:
        resolve_conf_data = resolve_conf_file.read()

    device_settings['Nameservers'] = {}
    count = 1
    for line in resolve_conf_data.split('\n'):
        if line.startswith('nameserver'):
            ip = line.replace('nameserver ', '').strip()

            if is_valid_ip_address(ip):  # IPV4 address
                device_settings['Nameservers']['DNS_' + str(count)] = str(ip)

    for count in range(1, 3):
        if 'DNS_' + str(count) not in device_settings['Nameservers']:
            device_settings['Nameservers']['DNS_' + str(count)] = None

    # get default gateway
    proc = subprocess.Popen(['/sbin/route', '-n'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (route_data, stderr) = proc.communicate()
    proc.wait()

    if isinstance(route_data, bytes):
        route_data = route_data.decode('utf-8', 'ignore')

    for line in route_data.split('\n'):
        if line.startswith('0.0.0.0'):  # IPv4
            split = [value.strip() for value in line.split('    ')]
            device_settings['IPV4']['Gateway'] = split[2]

    return device_settings


def is_wifi_available():
    return connman.is_technology_available('wifi')


def is_wifi_enabled():
    try:
        return connman.is_technology_enabled('wifi')
    except:
        return False


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
    interfaces = {}
    manager = connman.get_manager_interface()
    for entry in manager.GetServices():
        path = str(entry[0])
        dbus_properties = entry[1]
        if path.startswith(WIFI_PATH):
            wifi_settings = {
                'path': str(path),
                'Strength': int(dbus_properties['Strength']),
                'State': str(dbus_properties['State'])
            }

            security_props = dbus_properties['Security']
            if len(security_props) > 0:
                wifi_settings['Security'] = str(security_props[0])

            if 'hidden' not in path:
                wifi_settings['SSID'] = dbus_properties['Name'].encode('UTF-8')

            else:
                wifi_settings['SSID'] = '< Hidden (' + wifi_settings['Security'] + ') >'

            if not str(dbus_properties['State']) == 'idle':
                settings = extract_network_properties(dbus_properties)
                if settings:
                    wifi_settings.update(settings)

            # get Interface name and address
            eth_props = dbus_properties['Ethernet']
            wifi_settings['Interface'] = str(eth_props['Interface'])
            address = str(eth_props['Address'])
            wifi_settings['AdapterAddress'] = address
            if address not in interfaces:
                interfaces[address] = {}

            interfaces[address][wifi_settings['SSID']] = wifi_settings

    return interfaces


def wifi_connect(path, password=None, ssid=None, script_base_path=None):
    agent_needed = False
    process = None
    if password or ssid:
        agent_needed = True
        print('Starting Wireless Agent')
        with open('/tmp/preseed_data', 'w', encoding='utf-8') as key_file:
            if PY2:
                if not isinstance(password, unicode):
                    password = password.decode('utf-8')

                if not isinstance(ssid, unicode):
                    ssid = ssid.decode('utf-8')

            if password:
                print('Setting password')
                key_file.write(password)

            key_file.write('\n')

            if ssid:
                print('Setting SSID')
                key_file.write(ssid)

        agent_script = script_base_path + WIRELESS_AGENT
        process = subprocess.Popen([sys.executable, agent_script, 'fromfile'])

    print('Attempting connection to ' + path)
    service = connman.get_service_interface(path)

    connected = 1
    connection_attempts = 20
    while connected != 0 and connected < (connection_attempts + 1):
        try:
            service.Connect(timeout=15000)
            connected = 0
        except dbus.DBusException as e:
            if len(e.args) > 0 and e.args[0] == 'Not registered' and agent_needed:
                connected += 1
                time.sleep(1)
                print('Connection agent not started yet, waiting a second')

            else:  # another type of exception jump out of the loop
                connected = (connection_attempts + 1)
                print('DBusException Raised: ' + str(e))

    print('Connection to ' + path + ' : ' + str(connected == 0))
    if agent_needed:
        if process:
            process.kill()
        os.remove('/tmp/preseed_data')

    return connected == 0


def wifi_disconnect(path):
    service = connman.get_service_interface(path)
    try:
        service.Disconnect()
    except dbus.DBusException:
        print('DBusException disconnecting')


def wifi_remove(path):
    service = connman.get_service_interface(path)
    try:
        service.Remove()
    except dbus.DBusException:
        print('DBusException removing')


def get_connected_wifi():
    for address, wifis in get_wifi_networks().items():
        for ssid, value in wifis.items():
            if value['State'] in ('online', 'ready'):
                return value

    return {}


def has_network_connection(online):
    ethernet_settings = get_ethernet_settings()
    if ethernet_settings:
        if 'State' in ethernet_settings:
            if online:
                if ethernet_settings['State'] == 'online':
                    return True

            else:
                if ethernet_settings['State'] in ('ready', 'online'):
                    return True

        for internet_protocol in ['IPV4', 'IPV6']:
            if internet_protocol in ethernet_settings:
                if 'Method' in ethernet_settings[internet_protocol]:
                    if ethernet_settings[internet_protocol]['Method'].startswith('nfs_'):
                        if online:
                            return check_ms_ncsi_response()

                        else:  # if we on NFS we have network
                            return True

    for address, wifis in get_wifi_networks().items():
        for ssid in wifis.keys():
            info = wifis[ssid]
            if online:
                if info['State'] == 'online':
                    return True

            else:
                if info['State'] in ('ready', 'online'):
                    return True

    return False


def check_ms_ncsi_response():
    try:
        response = requests.get('http://www.msftncsi.com/ncsi.txt')
        if response.status_code == 200 and response.text == 'Microsoft NCSI':
            return True
    finally:
        return False


def parse_preseed():
    network_settings = None
    if os.path.isfile(PREESEED_LOCATION):
        with open(PREESEED_LOCATION, 'r', encoding='utf-8') as preseed_file:
            preseed_data = preseed_file.read()

        preseed_info = {}
        for entry in preseed_data.split("\n"):
            if entry.startswith('d-i network/') and len(entry.strip()) > 0:
                string = entry.replace('d-i network/', '')
                key = string[:string.find(' ')]
                string = string[string.find(' ') + 1:]
                _type = string[:string.find(' ')]
                value = string[string.find(' ') + 1:]
                preseed_info[key] = value

        # convert preseed format into the same format we are using in the rest of the code
        network_settings = {
            'Interface': preseed_info['interface']
        }

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
    return osmc_systemd.is_service_enabled(WAIT_FOR_NETWORK_SERVICE)


def toggle_wait_for_network(state):
    if state:
        osmc_systemd.update_service(WAIT_FOR_NETWORK_SERVICE, 'enable')

    else:
        osmc_systemd.update_service(WAIT_FOR_NETWORK_SERVICE, 'disable')


def is_tethering_ethernet():
    return connman.is_technology_tethering('ethernet')


def is_tethering_wifi():
    return connman.is_technology_tethering('wifi')


def get_tethering_ssid():
    info = connman.get_technology_info('wifi')
    if 'TetheringIdentifier' in info:
        return str(info['TetheringIdentifier'])

    return None


def get_tethering_passphrase():
    info = connman.get_technology_info('wifi')
    if 'TetheringPassphrase' in info:
        return str(info['TetheringPassphrase'])

    return None


def tethering_enable(technology, ssid, passphrase):
    return connman.tethering_enable(technology, ssid, passphrase)


def tethering_disable():
    if is_tethering_wifi():
        return connman.tethering_disable('wifi')

    if is_tethering_ethernet():
        return connman.tethering_disable('ethernet')


def is_valid_ipv4_address(ip):
    try:
        socket.inet_aton(ip)
        return True
    finally:
        return False


def is_valid_ipv6_address(ip):
    try:
        socket.inet_pton(socket.AF_INET6, ip)
        return True
    finally:
        return False


def is_valid_ip_address(ip):
    return is_valid_ipv4_address(ip) or is_valid_ipv6_address(ip)


def is_ftr_running():
    return osmc_systemd.is_service_running('ftr')
