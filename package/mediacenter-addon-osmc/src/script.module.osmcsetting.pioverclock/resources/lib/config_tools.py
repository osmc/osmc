
# Standard Modules
import StringIO
import ConfigParser
import subprocess
from collections import OrderedDict
import re
from CompLogger import comprehensive_logger as clog

class MultiOrderedDict(OrderedDict):
    '''
        A new type of dictionary that handles the concatenation of multiple entries when read from the config,
        and the addition of new multiple entries on write.
    '''


    def __setitem__(self, key, value):

        alias = {      'device_tree_overlay'    :   'dtoverlay', 
                        'device_tree_param'     :   'dtparam',
                        'device_tree_params'    :   'dtparams'
                        }

        if key in alias:
            key = alias[key]
            
        #print 'mod: key value provided, \n\t%s = %s' % (key, value)
        if isinstance(value, list) and key in self:
            #print 'mod: instance is list and key in self'
            if not isinstance(self[key], list):
                #print 'mod: existing key is not list'
                new_key = key + '[----]0' 
                if new_key in self:
                    #print 'mod: new_key (%s) in self, extending' % new_key
                    self[new_key].extend(value)
                else:
                    #print 'mod: new_key (%s) not in self, creating and extending'
                    if isinstance(self[key], str):
                        #print 'mod: existing key is a string, splitting into list'
                        self[new_key] = self[key].split('\n')
                    else:
                        #print 'mod: existing key NOT a string'
                        self[new_key] = [self[key]]
                    self[new_key].extend(value)
                    #print 'mod: new_key extended, \n\t%s' % self[new_key]
            else:
                #print 'mod: existing key value is list, extending'
                self[key].extend(value)
        else:
            #print 'mod: instance is not list or key not in self, setting as normal'
            super(MultiOrderedDict, self).__setitem__(key, value)

        #print '\n'


def grab_configtxt(config_location):

    '''
        Creates a parser that reads the config.txt. Establishes a section called [osmc]
    '''

    # open the config file read the contents into a long string
    with open(config_location,'r') as f:

        lines = f.readlines()

        # santise the text
        sanitised_lines = []
        for line in lines:
            if line:
                if '=' in line[1:] and not line.startswith('#'):
                    sanitised_lines.append(line)

        # add [root] to the front to create a root section for the parser to read
        long_string = '[osmc]\n' + ''.join(sanitised_lines)

    # sanitise the string

    # put the string into a stringIO to allow the parser to read it like a file
    long_string_file = StringIO.StringIO(long_string)
    
    # instantiate a config parser
    parser = ConfigParser.RawConfigParser(dict_type=MultiOrderedDict)

    # force all data to be written as a string
    parser.optionxform = str

    # read the stringIO into the config parser, file_config represents the previous config data
    parser.readfp(long_string_file)

    return parser


@clog()
def read_config(config_location, parser_provided=False, return_the_parser=False):

    '''
        Reads the config and retrieves the settings in the form of a dict with the settings {name: value}.
        Multiple values for the same key are concatenated into a single string with a newline separator.
    '''

    # grab the parser if it isnt provided
    if parser_provided:
        parser = parser_provided
    else:
        parser = grab_configtxt(config_location)

    # retrieve all the settings in the config, in tuples with the settings (name, value)
    settings_raw = parser.items('osmc')

    settings = dict(settings_raw)

    del parser

    if return_the_parser:
    
        return settings, parser

    else:

        return settings

@clog()
def write_config(config_location,  changes={}):

    '''
        Write the changes back to the config.txt.

        'changes' should be a dictionary with the setting name as the key, and the new setting value as the value.
        'changes' can also include a key 'remove', the value of which is a list with the settings to remove from the 
        config file.
        Multiple entries with the same key are returned as a list. To remove a setting from a multiple entry, the string 
        must include [remove].
    '''

    # grab the parser if it isnt provided
    blotter = grab_configtxt(config_location)
    #print '-----------------------------------------------------'

    # force all data to be written as a string
    blotter.optionxform = str
    #print '-----------------------------------------------------'

    # loop through the changes and make the change to the config
    for setting, value in changes.iteritems():

        #print 'tool: changed setting = %s' % setting
        #print 'tool: changed value = type(%s) %s' % (type(value),value)

        # if the setting is set to be removed, then remove those entries from the config
        if value == 'remove':

            #print 'tool: setting removed'

            # this separate rule is required for lirc-rpi because it often carries parameters
            if setting == 'lirc-rpi-overlay':

                for k, v in blotter.items('osmc'):
                    if 'lirc-rpi' in k:
                        blotter.remove_option('osmc', k)
                        break

            else:

                blotter.remove_option('osmc', setting)

            continue

        if isinstance(value, list):
            for x in value:
                if 'lirc-rpi' in x:
                    # if lirc-rpi is in the value, then remove the existing entry in the blotter as it will be overwritten
                    for k, v in blotter.items('osmc'):
                        if isinstance(v, str):
                            if 'lirc-rpi' in v:
                                v += '\n'
                                new_v = v[:v.index('lirc-rpi')] + v[v[v.index('lirc-rpi'):].index('\n') + v.index('lirc-rpi'):]
                                new_v = new_v[:len(v)-2]
                                blotter.set('osmc', k, new_v)
                                break

        # otherwise, write the new value to the setting
        #print 'tool: setting value in blotter'
        blotter.set('osmc', setting, value)

    #print '-----------------------------------------------------'
    tally = 0

    # multiple entries: 
    #           expands a concatenated string into individual 'repeated' settings, 
    #           expands a list of multiple settings into individual 'repeated' settings
    # multiple entries are given their own unique keys with [----]integer appended
    # the original settings (string OR list) are removed
    items = blotter.items('osmc')
    for setting, value in items:
        #print 'tool: blotter items: setting = %s' % setting 
        #print 'tool: blotter items: value = %s' % value
        if isinstance(value, str):
            if '\n' in value:
                dupes = value.split('\n')
                #print 'tool: value is string and newline present, dupes = %s' % dupes
                for dupe in dupes:
                    tally += 1
                    blotter.set('osmc', setting + '[----]' + str(tally), dupe)
                    #print 'tool: blotter set: \n\t%s = %s\n' % (setting + '[----]' + str(tally), dupe)
                blotter.remove_option('osmc', setting)
                #print 'tool: removed original setting from blotter, \n\t%s' % setting
        if isinstance(value, list):
            #print 'tool: value is a list, splitting to individual lines'
            for dupe in value:
                tally += 1
                blotter.set('osmc', setting + '[----]' + str(tally), dupe)
                #print 'tool: blotter set: \n\t%s = %s\n' % (setting + '[----]' + str(tally), dupe)
            blotter.remove_option('osmc', setting)
            #print 'tool: removed original setting from blotter, \n\t%s' % setting
    
    #print '-----------------------------------------------------'
    # this next section handles :
    #           the removal of multiple entry items
    #           the removal of the unique appendices

    # grab all the items from the blotter
    items = blotter.items('osmc')

    # list to hold all the new lines from the blotter
    new_lines = []

    # pattern for replacing the unique appendix on settings that have them
    pattern = re.compile(r"\[----]\d*", re.IGNORECASE)

    # list of values
    values = [v for k,v in items]

    # this is a list of all the multiple entry items that arent tagged for removal
    new_items = [(k, v) for k, v in items if all([str(v) + '[remove]' not in values, '[remove]'  not in str(v)])]

    # remove the unique appendices
    for k,v in new_items:
        new_key = re.sub(pattern, '', k)
        new_lines.append('%s=%s' % (new_key, v))

    # remove duplicate lines (this can occur when there are multiple entries in both the string and list)
    final_lines = list(set(new_lines))

    # temporary location for the config.txt
    tmp_loc = '/var/tmp/config.txt'

    # write the long_string_file to the config.txt
    with open(tmp_loc,'w') as f:
        for line in final_lines:
            print 'final line: %s' % line
            f.write(line.replace(" = ","="))
            f.write('\n')

    # copy over the temp config.txt to /boot/ as superuser
    subprocess.call(["sudo", "mv",  tmp_loc, config_location])


if ( __name__ == "__main__" ):

    # this is used for testing
    
    # location of the text config file
    file_loc = '/home/kubkev/Documents/config.txt'

    # #print the config as read by the parser
    op = read_config(file_loc)
    #print 'ORIGINAL: %s' % op

    # apply this test set of changes
    changes = {'dtoverlay' : ['frts','hifiberry-dac-overlay[remove]', 'iqaudio-dac-overlay[remove]', 'hifiberry-digi-overlay[remove]', 'w1-gpio-overlay[remove]', 'w1-gpio-pullup-overlay[remove]', 'lirc-rpi-overlay']}

    write_config(file_loc, changes)




