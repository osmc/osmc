import dbus


CONNMAN_OBJECT_PATH = 'net.connman'



def is_technology_available(technology):
    if get_technology_info(technology) is not None:
        return True
    return False


def is_technology_enabled(technology):
    if get_technology_info(technology) is not None:
        technology_dict = get_technology_info(technology)
        if technology_dict['Powered']:
            return True
    return False


# queries DBUS to see if the specified technology is detected, returns a dictionary with the details if not returns None
def get_technology_info(technology):
    manager = get_manager_interface();
    technologies = manager.GetTechnologies()
    for t in technologies:
        if t[0] == '/net/connman/technology/' + technology:
            return t[1]


def toggle_technology_state(technology, state):
    bus = dbus.SystemBus()
    technology_interface = dbus.Interface(bus.get_object(CONNMAN_OBJECT_PATH, '/net/connman/technology/' + technology),
                                'net.connman.Technology')
    try:
        technology_interface.SetProperty('Powered', state)
    except dbus.DBusException, e:
        print('DBus Exception ' + str(e))
        return False
    return True


def get_manager_interface():
    bus = dbus.SystemBus()
    try:
        return dbus.Interface(bus.get_object(CONNMAN_OBJECT_PATH, '/'), 'net.connman.Manager')
    except dbus.DBusException, error:
        print('Could not get connman manager interface')


def get_service_interface(path):
    bus = dbus.SystemBus()
    try:
        return dbus.Interface(bus.get_object(CONNMAN_OBJECT_PATH, path), 'net.connman.Service')
    except dbus.DBusException, error:
       print('Could not get connman service interface')


def get_technology_interface(technology):
    bus = dbus.SystemBus()
    try:
        return dbus.Interface(bus.get_object(CONNMAN_OBJECT_PATH, '/net/connman/technology/'+technology),
                              "net.connman.Technology")
    except dbus.DBusException, error:
        print('Could not get connman technology' + technology + 'interface')

