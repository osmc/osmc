import subprocess


def is_service_running(service_name):
    process = subprocess.call(['sudo', '/bin/systemctl', 'is-enabled', service_name])
    if process == 0:
        enabled = True
    else:
        enabled = False
    process = subprocess.call(['sudo', '/bin/systemctl', 'is-active', service_name])
    if process  == 0:
        active = True
    else:
        active = False
    return enabled and active


def toggle_service(service_name, enable):
    if enable:
        subprocess.call(['sudo', '/bin/systemctl', 'enable', service_name])
        subprocess.call(['sudo', '/bin/systemctl', 'start',  service_name])
        
    else:
        subprocess.call(['sudo', '/bin/systemctl', 'disable', service_name])
        subprocess.call(['sudo', '/bin/systemctl', 'stop',    service_name])
