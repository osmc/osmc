


'''
When Reading the settings from the file
1. Read the file 
2. Sanitise the file
    - ignore comments
    - ignore new lines 
    - combine dtoverlays plus parameters into a single line
    - if a line misses an equals sign, then add one, we will ignore this datapoint anyway
3. Store the new data into a StringIO file.
4. Pass this file through the ConfigObj
5. Retrieve the parameters into a dict

When Writing the settings to the file
1. Repeat the read to get the ConfigObj
2. Make changes as necessary to the settings 
    - combine dtoverlays into a single line
3. Write the ConfigObj back to a stringIO
4. Sanitise the string 
    - strip out the superfluous equals signs and _SPACE_'s
    - remove the spaces around the = signs
5. Write the stringIO back into the file 
    - write to a temp file, then use CLI to escalate privelages to sudo cp the file back into /boot
    



'''
from configobj import ConfigObj
import StringIO
import subprocess
import sys


def read_and_sanitise_file(file_loc='C:\\temp\\config.txt'):
    ''' Takes the config.txt and constructs a StringIO object that will work well with the ConfigObj parser. '''

    overlay_storage = []

    newlines = []

    aliases = {     'device_tree_overlay'   :   'dtoverlay', 
                    'device_tree_param'     :   'dtparam',
                    'device_tree_params'    :   'dtparams',
                    }

    with open(file_loc, 'r') as f:

        lines = f.readlines()

        for line in lines:

            line = line.strip()

            print '>>> ' + line

            for original, alias in aliases.iteritems():
                line = line.replace(original, alias)


            # pass through all commented lines and newlines
            if line.startswith('#') or line == '\n' or not line:
                newlines.append(line)
                print 'pass through\n'
                continue

            # add dtparams to overlay_storage
            if overlay_storage and 'dtparam' in line:
                line = line.replace(' ','')
                overlay_storage.append(line[line.index('=')+1:].strip())
                print 'append param\n'
                continue

            # if the line is not a new dtparam, then roll those lines up together and appened to newlines
            elif overlay_storage:
                print 'combining overlay block'
                overlay_root = overlay_storage.pop(0)
                if overlay_storage:
                    overlay_root = overlay_root + '='
                else:
                    if ':' in overlay_root:
                        overlay_root = overlay_root.replace(':','=')
                    else:
                        overlay_root = overlay_root + '=PLACEHOLDER'
                overlay_oneline = overlay_root + ','.join(overlay_storage)
                newlines.append(overlay_oneline)
                overlay_storage = []
                print overlay_oneline
                print 'combined params\n'

            elif 'dtparam' in line: # ignore the line, dtparam lines should not be placed anywhere but after dtoverlay entries
                print 'ignoring line\n'
                continue

            # if this is a new dtoverlay block, add the starting line to overlay storage, but first make sure that it is unique using the overlay count
            if 'dtoverlay=' in line:
                line = line.replace(' ','')
                overlay_storage.append(line.strip().replace('dtoverlay=', 'dtoverlay_||_'))     # _||_ this is merely a place holder to indicate dtoverlays
                print 'starting overlay block\n'
                continue

            # if there is no equals sign, then remove any spaces from the string, and add an equals sign, THIS MUST BE REVERSED WHEN WRITING BACK
            if line and '=' not in line:
                newlines.append(line.replace(' ', '_SPACE_')+'=NO_EQUALS_SIGN')
                print 'no equals sign\n'
                continue

            newlines.append(line)
            print 'added: %s\n' % line

        # take care of any orphaned dtoverlay blocks
        if overlay_storage:
            print 'handling ophaned overlay blocks: %s' % overlay_storage
            overlay_root = overlay_storage.pop(0)
            if overlay_storage:
                overlay_root = overlay_root + '='
            else:
                if ':' in overlay_root:
                    overlay_root = overlay_root.replace(':','=')
                else:
                    overlay_root = overlay_root + '=PLACEHOLDER'
            overlay_oneline = overlay_root + ','.join(overlay_storage)
            newlines.append(overlay_oneline)
            overlay_storage = []
            print overlay_oneline
            print 'combined params\n'


    # handle duplicates by commenting out the earlier entries, rPi uses the LAST entry, so we should be consistent with that
    
    # first reverse the newlines
    newlines = newlines[::-1]

    # create a new list that contains just the keys of the config, storing NONE for any line that doesn't have an equals sign
    split_list = [None if any(["=" not in x, x.startswith('#')]) else x[:x.index('=')] for x in newlines]
    
    # this list will house our unique values
    uniques  = []

    # cycle through the split list, ignoring the None's and storing just the unique keys in the uniques list
    # if the key is already in the uniques list, then edit the corresponding item in newlines by adding a # on the front (turning it into a comment)
    for i, key in enumerate(split_list):
        if key is None:
            continue

        if key in uniques:
            newlines[i] = '#' + newlines[i]
        else:
            uniques.append(key)

    # reverse the newlines list back to its original order
    newlines = newlines[::-1]


    # put the string into a stringIO to allow the Config parser to read it like a file
    return StringIO.StringIO('\n'.join(newlines))


def write_to_config_file(config_stringIO, export_location='C:\\temp\\temp.txt'):

    with open(export_location, 'w') as f:
        config_stringIO.seek(0)
        for line in config_stringIO.readlines():
            print line
            # this identifies dtoverlays
            if '_||_' in line:

                # if the dtoverlay contains more than one equals sign, then it must have parameters, so replace the first equals sign with a colon
                if line.count('=') > 1:
                    i = line.index('=')
                    line = line[:i] + ':' + line[i + 1:]

                # for dtoverlays, replace the flag, _||_ with an equals sign
                line = line.replace('_||_','=')

            # replace the other changes made during sanitisation
            line = line.replace(' = ', '=').replace('_SPACE_', ' ').replace('=NO_EQUALS_SIGN','').replace('=PLACEHOLDER','')
            f.write(line)



def apply_changes_to_configtxt(changes, file_loc='C:\\temp\\config.txt'):
    ''' Reads the existing config.txt, santises the data then parses the info into a configObject, applies the desired changes to that object,
        then converts that object to a stringIO object, reverses the sanitisation, and writes it back to the config.txt  '''

    sanitised_file = read_and_sanitise_file(file_loc)

    config_dict = ConfigObj(infile=sanitised_file, write_empty_values=True, list_values=False)

    print changes

    print ''

    print config_dict

    for key, value in changes.iteritems():

        if key == 'dtoverlay':

            if not isinstance(value, list):
                continue

            for dtoverlay_item in value:
                
                if ':' in dtoverlay_item:
                    true_key = 'dtoverlay_||_' + dtoverlay_item[:dtoverlay_item.index(":")]
                    true_val = dtoverlay_item[dtoverlay_item.index(":")+1:]
                else:
                    true_key = 'dtoverlay_||_' + dtoverlay_item.replace('[remove]','')
                    true_val = "PLACEHOLDER"

                if '[remove]' in dtoverlay_item:

                    if true_key in config_dict:
                        del config_dict[true_key]

                else:
                    config_dict[true_key] = true_val
            continue
        
        if value == 'remove':
            del config_dict[key]
            continue

        config_dict[key] = value

    blotter = StringIO.StringIO()
    config_dict.write(blotter)

    print ''
    print config_dict

    # temporary location for the config.txt
    tmp_loc = '/var/tmp/config.txt'
    tmp_loc = 'C:\\temp\\temp.txt'
    write_to_config_file(blotter, tmp_loc)


    # copy over the temp config.txt to /boot/ as superuser
    # subprocess.call(["sudo", "mv",  tmp_loc, file_loc])


def prepare_config_dict_for_addon(config_dict):
    ''' Takes the config (dictionary) and prepares it for use in the addon.
        This entails:
                - combining all the dtoverlay lines into a single string separated by a /n under the key "dtoverlay"
    '''
    
    combined_overlays = []
    remove_keys = []
    for k, v in config_dict.iteritems():
        if 'dtoverlay' in k:
            remove_keys.append(k)
            new_dtoverlay = k.replace('dtoverlay_||_', '')

            if v != 'PLACEHOLDER':
                new_dtoverlay = new_dtoverlay + ':' + v

            combined_overlays.append(new_dtoverlay)

    if combined_overlays:
        config_dict['dtoverlay'] = '\n'.join(combined_overlays)

    for key in remove_keys:
        del config_dict[key]

    return config_dict


def retrieve_settings_from_configtxt(file_loc='C:\\temp\\config.txt'):
    ''' Reads the config.txt, santises the data, parses the data into a configObject, perpares that object for use in the addon, and returns that object '''

    sanitised_file = read_and_sanitise_file(file_loc)

    config_dict = ConfigObj(infile=sanitised_file, write_empty_values=True, list_values=False)

    return prepare_config_dict_for_addon(config_dict)


def test():
    changes = {'dtoverlay' : [
    'hifiberry-dac-overlay[remove]', 
    'iqaudio-dac-overlay[remove]', 
    'hifiberry-digi-overlay', 
    'w1-gpio-overlay[remove]', 
    'w1-gpio-pullup-overlay[remove]', 
    'lirc-rpi-overlay:gpio_out_pin=9999999,gpio_in_pin=23,gpio_in_pull=shiiiiit'
    ]
    }

    apply_changes_to_configtxt(changes)


# device_tree_overlay=lirc-rpi-overlay

# dtparam=gpio_out_pin=16
# dtparam=gpio_in_pin=17
# dtparam=gpio_in_pull=down

# dtoverlay=iqaudio-dac-overlay
# dtoverlay=hifiberry-digi-overlay:dunk=shart,funck=darts
