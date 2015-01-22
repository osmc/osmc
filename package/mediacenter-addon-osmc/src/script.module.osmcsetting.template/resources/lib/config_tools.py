

import StringIO
import ConfigParser
import re
import time

def load_sysconfig():

    # open the config file
    with open('/boot/config.txt','r') as f:

        # call the entire contents of the file the "root" section and read the file into the initial string
        ini_str = '[root]\n' + f.read()

    # dont know if this is needed
    # CHECK IN TESTING
    ini_fp = StringIO.StringIO(ini_str)
    
    # instantiate a config parser
    file_config = ConfigParser.RawConfigParser()

    # read the string into the config parser, file_config represents the previous config data
    file_config.readfp(ini_fp)

    # create the new dictionary that will hold the settings and values
    config = {}

    # these are the initial default values of the system config settings.
    default_values = {
                        "sys.config.freq.arm"           :   800,
                        "sys.config.freq.core"          :   250,
                        "sys.config.freq.gpu"           :   250,
                        "sys.config.freq.isp"           :   250,
                        "sys.config.freq.sdram"         :   400,
                        "sys.config.freq.overvolt"      :   0,
                        "sys.config.disable.overscan"   :   "1",
                        "sys.config.decode.mpg2"        :   "",
                        "sys.config.decode.wvc1"        :   "",
                        "sys.config.decode.dts"         :   "",
                        "sys.config.decode.ac3"         :   "",
                    }
 

    # if settings were provided, check if the key doesnt start with "sys.config", and if it doesnt
    # then add that key to the new dict with an empty string as the value
    # if settings != None:

    #     for item in settings.items():

    #         if not item[0].startswith("sys.config."):

    #             config[item[0]] = ""


    # every key that doesnt start with "sys.config" should have their values brought over from the previous config file
    for k, v in config.iteritems():

        if not k.startswith("sys.config."):

            try:

                config[k] = file_config.get("root",k)

            except:

                pass


    keymap = {
                "sys.config.freq.arm"           : "arm_freq",
                "sys.config.freq.core"          : "core_freq",
                "sys.config.freq.isp"           : "isp_freq",
                "sys.config.freq.gpu"           : "gpu_freq",
                "sys.config.freq.sdram"         : "sdram_freq",
                "sys.config.freq.overvolt"      : "over_voltage",
                "sys.config.disable.overscan"   : "disable_overscan",
                "sys.config.decode.mpg2"        : "decode_MPG2",
                "sys.config.decode.wvc1"        : "decode_WVC1",
                "sys.config.decode.dts"         : "decode_DTS",
                "sys.config.decode.ac3"         : "decode_DDP",
            }

    # copy over all the config data from the file to the new dict
    for k, v in keymap.iteritems():

        try:

            config[k] =  file_config.get("root",v)

        except:

            config[k] = default_values.get(k, '')


    # return the config dictionary with all the system config values from the existing config file
    return config


def remove_comments(line):

    '''
        removes the section of the line after a hash symbol
    '''

    try:
        i = line.index('#')
        return line[:i]
    except:
        return line


def edit_configtxt(configtxt, changes={}):

    # open the file and read the lines into a list
    with open(configtxt, 'r') as f:
        raw_lines = f.readlines()

        # THIS PART IS FOR TESTING, I DONT WANT TO BORK ANYONES CONFIG
        # create a backup of the existing config file
        backup_name = configtxt.replace(".txt","") + "_backup_" + str(int(time.time())) + ".txt"

        with open(backup_name, 'w') as w:
            w.writelines(raw_lines)


    # remove the commented out sections
    lines = [remove_comments(line) for line in raw_lines]

    # combine the lines into a single string
    line_string = ' '.join(lines)

    # this is the partial pattern to help find the setting in the long string
    pattern = '\s*=\s*([^\s]*)'

    # cycle through all the changes and update the long string
    for key, setting in changes.iteritems():

        # this is the setting we will be adding to the long string
        new_string = str(key) + "=" + str(setting)

        # this pattern will help carve out the setting if it exists in the long string already
        tmp_pattern = re.compile("(" + setting + pattern + ")")

        # find the pattern in the long string
        existing_setting = re.findall(tmp_pattern, line_string)

        # if it is found then carve it out
        if existing_setting:
                
            # carve the setting out of the string
            group, setting = existing_setting[0]
            start = line_string.index(group)
            end = start + len(group)

            # carve up the long string
            ex_string = line_string[:start] + line_string[end:]
            
            # insert the new setting
            line_string = ' '.join([ex_string, new_string])

        # if the patter isnt found, then just add the setting to the end of the long string
        else:

            if key = 'unknown':
                new_string = setting.replace(";", " ")

            line_string = ' '.join([line_string, new_string])

    # overwrite the file with the new long string
    with open(configtxt, 'w') as f:
        f.write(line_string)



def read_configtxt(configtxt, known_settings=[]):

    read_settings = {}
    unknown_settings = []

    # open the file and read the lines into a list
    with open(configtxt, 'r') as f:
        raw_lines = f.readlines()

    # remove the commented out sections
    lines = [remove_comments(line) for line in raw_lines]

    # combine the lines into a single string
    line_string = ' '.join(lines)

    # this is the partial pattern to help find the setting in the long string
    pattern = '(([^\s]*)\s*=\s*([^\s]*))'

    # this pattern will help identify the settings in the long string already
    tmp_pattern = re.compile("(" + setting + pattern + ")")

    # find the pattern in the long string
    existing_settings = re.findall(tmp_pattern, line_string)

    for setting in existing_settings:
            
        # carve the setting out of the string
        group, setting, value = setting

        if setting in known_settings:
            read_settings[setting] = value

        else:
            unknown_settings.append(group)

    # if the patter isnt found, then just add the setting to the end of the long string
    else:

        READING CONFIG FAILED

    read_settings['unknown'] = ';'.join(unknown_settings)

    return read_settings




def test_sysconfig():

    # open the config file
    with open('C:\\Temp\\config.txt','r') as f:

        # call the entire contents of the file the "root" section and read the file into the initial string
        ini_str = '[root]\n' + f.read()

    # dont know if this is needed
    # CHECK IN TESTING
    ini_fp = StringIO.StringIO(ini_str)
    
    # instantiate a config parser
    file_config = ConfigParser.RawConfigParser()

    # read the string into the config parser, file_config represents the previous config data
    file_config.readfp(ini_fp)

    print file_config.options('root')



if ( __name__ == "__main__" ):

    test_sysconfig()