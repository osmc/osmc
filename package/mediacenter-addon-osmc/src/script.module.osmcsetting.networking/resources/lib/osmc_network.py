import connman
import dbus
import time
import subprocess
import re


ETHERNET_PATH = '/net/connman/service/ethernet'

# this is wher we read NFS network info from this is the current running config
RUNNING_NETWORK_DETAILS_FILE = '/proc/cmdline'
# but we want to update here - this gets copied to /proc/ as part og boot
UPDATE_NETWORK_DETAILS_FILE = '/boot/cmdline'
PREESEED_TEMP_LOCATION = '/tmp/preseed.tmp'
PREESEED_LOCATION = '/boot/preseed.cfg'

manager = connman.get_manager_interface()

def is_ethernet_enabled():
    return connman.is_technology_enabled('ethernet')


def toggle_ethernet_state(state):
    connman.toggle_technology_state('ethernet', state)


def get_network_settings():
    for entry in manager.GetServices():
        path = entry[0]
        dbus_properties = entry[1]
        if path.startswith(ETHERNET_PATH):
            props = {'path': path}
            # get IPv4 Data
            ipv4_props = dbus_properties['IPv4']
            props['Method'] = str(ipv4_props['Method'])
            props['Address'] = str(ipv4_props['Address'])
            props['Netmask'] = str(ipv4_props['Netmask'])
            props['Gateway'] = str(ipv4_props['Gateway'])
            # Get  DNS Servers
            nameservers = dbus_properties['Nameservers']
            count = 1
            for nameserver in nameservers:
                props['DNS_' + str(count)] = str(nameserver)
                count += 1
            # get state
            props['State'] = str(dbus_properties['State'])
            # get Interface name
            eth_props = dbus_properties['Ethernet']
            props['Interface'] = str(eth_props['Interface'])
            return props

    # if we are here we have not detected a ethernet connection check for NFS
    cmdline_data = open(RUNNING_NETWORK_DETAILS_FILE, 'r').read()
    nfs_found = False
    ip_value = None
    for cmdline_value in cmdline_data.split(' '):
        if cmdline_value == 'root=/dev/nfs':
            nfs_found = True
        # grab the ip= value from cmdline
        if cmdline_value.startswith('ip='):
            ip_value = cmdline_value[3:]

    if nfs_found:
        if ip_value == 'dhcp':
            nfs_settings = get_non_connman_connection_details()
            nfs_settings['Method'] = 'nfs_dhcp'
        else:
            nfs_settings = split_nfs_static_cmdlline(ip_value)
            nfs_settings['Method'] = 'nfs_manual'

        nfs_settings['Interface'] = 'eth0 (NFS)'
        nfs_settings['State'] = 'online'
        nfs_settings['path'] = 'nfs'
        if not has_internet_connection():
            nfs_settings['State'] = 'ready'
        return nfs_settings

    return None


def apply_network_changes(settings_dict):
    if settings_dict['Method'] in ['manual', 'dhcp']:  # non NFS setup
        path = settings_dict['path']
        service = connman.get_service_interface(path)
        ipv4_configuration = {'Method': make_variant(settings_dict['Method']),
                              'Address': make_variant(settings_dict['Address']),
                              'Netmask': make_variant(settings_dict['Netmask']),
                              'Gateway': make_variant(settings_dict['Gateway'])}
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
        update_preseed_file(settings_dict)
    elif settings_dict['Method'].startswith('nfs_'):
        ip_value = None
        if settings_dict['Method'] == 'nfs_dhcp':
            ip_value = 'dhcp'
        if settings_dict['Method'] == 'nfs_manual':
            ip_value = create_cmdline_nfs_manual_string(settings_dict)
        if ip_value:
            update_cmdline_file(UPDATE_NETWORK_DETAILS_FILE, 'ip', ip_value)
        update_preseed_file(settings_dict)


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
    cmdline_file = open(file_path, 'w')
    cmdline_file.write(updated_cmdline)
    cmdline_file.close()


'''
  | Address   || gateway   | netmask     |    |    |   | DNS_1 | DNS_2
  192.168.1.20::192.168.1.1:255.255.255.0:osmc:eth0:off:8.8.8.8:8.8.4.4
'''


def split_nfs_static_cmdlline(value):
    connection_details = value.split(':')
    print connection_details
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


def has_internet_connection():
    try:
        subprocess.Popen(['/bin/ping', '-c1', '-w2', '8.8.8.8'], stdout=subprocess.PIPE).stdout.read()
        return True
    except:
        return False


def update_preseed_file(settings_dict):
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

    # write the required network options to the temp file
    if 'eth' in settings_dict['path'] or settings_dict['path'] == 'nfs':
        preseed_file.write('d-i network/interface string eth\n')
    if 'dhcp' in settings_dict['Method']:
        preseed_file.write('d-i network/auto boolean true\n')
    if 'manual' in settings_dict['Method']:
        preseed_file.write('d-i network/auto boolean false\n')
        preseed_file.write('d-i network/ip string ' + settings_dict['Address'] + '\n')
        preseed_file.write('d-i network/mask string ' + settings_dict['Netmask'] + '\n')
        preseed_file.write('d-i network/gw string ' + settings_dict['Gateway'] + '\n')
        if 'DNS_1' in settings_dict:
            preseed_file.write('d-i network/dns1 string ' + settings_dict['DNS_1'] + '\n')
        if 'DNS_2' in settings_dict:
            preseed_file.write('d-i network/dns2 string ' + settings_dict['DNS_2'] + '\n')
        preseed_file.write('d-i network/gw string ' + settings_dict['Gateway'] + '\n')
    preseed_file.close()
    subprocess.call(['sudo', 'mv', PREESEED_TEMP_LOCATION, PREESEED_LOCATION])

