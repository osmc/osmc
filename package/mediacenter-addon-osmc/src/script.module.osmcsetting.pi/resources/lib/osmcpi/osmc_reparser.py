# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.pi

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.

    DICT OF ITEMS IN CONFIG WHICH AFFECT THIS KODI SETTING

    kodi_setting_id:    {

        KODI_ITEM:
                {
                config_get_patterns: [
                                LIST OF DICTS WITH AN IDENTIFY REGEX AND AN EXTRACT REGEX
                                IDENTIFY REGEX'S ARE THERE TO SPECIFICALLY FIND THE LINES
                                EXTRACT REGEXES ARE THERE TO GET THE SPECIFIC SETTING VALUE
                            ],


                config_set :    FUNCTION TO CHANGE THE SETTING IN THE CONFIG,
                                this code should cycle through the config line list, search for
                                each config_key, then substitute that line with the new one

                config_validation:  FUNCTION TO VALIDATE THE VALUE FROM THE CONFIG,
                                    this also converts the value into a form kodi settings
                                    will recognise
                                    i.e. it should convert binary 0:1 to 'false'|'true'

                kodi_set:   additional function that can be used to do more specific conversion
                            from the config to kodi. for instance, if a single setting in kodi
                            relies upon two different settings in the config, this can combine them

                setting_stub:   a stub of the string that will replace the lines in the config.txt,
                                the value is inserted into it
                },
    # ...

    NEEDS A REMOVE LIST
    NEEDS A FINAL CHECK FOR HDMI_SAFE to make sure the entries related to it are removed if
    it is on hdmi_safe can be checked in the dict immediately after the settings are extracted
    from kodi that way the other values can be set and override the ones taken from kodi

"""

import re
import subprocess
import sys
from io import open

PY2 = sys.version_info.major == 2


def config_to_kodi(settings, config):
    """
        Takes the existing config and uses the protocols in the MASTER_SETTINGS to
        extract the settings for use in kodi.

        Returns a dictionary of kodi settings and kodi values.
    """

    extracted_settings_for_kodi = {}

    for setting, protocols in settings.items():
        value = general_config_get(config, **protocols)
        extracted_settings_for_kodi[setting] = value

    # The gpio_pin_in has a default value of 17 when the lirc-rpi overlay is present.
    # Some configs may not report a gpio_in_pin for this reason.
    # We must presume if the lirc-rpi overlay is found that the gpio_pin setting
    # in kodi should be set at 17.
    lirc_is_present = extracted_settings_for_kodi['lirc-rpi-overlay'] != 'defunct'
    gpio_in_pin_is_not_present = extracted_settings_for_kodi['gpio_in_pin'] == 'defunct'
    gpio_pin_is_not_present = extracted_settings_for_kodi['gpio_pin'] == '0'
    if lirc_is_present & gpio_in_pin_is_not_present & gpio_pin_is_not_present:
        extracted_settings_for_kodi['gpio_pin'] = '17'

    return extracted_settings_for_kodi


def general_config_get(config, config_get_patterns, config_validation, kodi_set, default, **kwargs):
    """
        Searches the config.txt for specific settings and returns the values when they are found.
        Uses the validation and kodi_set protocols to convert the config settings into
        kodi settings.

        Returns a valid kodi setting value.
    """
    results = []

    # the config is reviewed in reverse order so that the final of any duplicate settings is
    # first in the results list this is consistent with how the rPi does it's config parsing
    for line in config[::-1]:
        # ignore blank lines
        if not line.strip():
            continue

        # ignore commented out lines
        if line.strip().startswith("#"):
            continue

        # strip the line of any inline comments
        if "#" in line:
            line = line[: line.index("#")]

        for pair in config_get_patterns:
            matched = re.search(pair["identify"], line, re.IGNORECASE)

            if matched:
                raw_value = re.search(pair["extract"], line, re.IGNORECASE)

                if raw_value:
                    result = config_validation(raw_value.group(1))

                    if result is not None:
                        results.append(result)

    kodi_setting = kodi_set(results)
    if kodi_setting is None:
        kodi_setting = retrieve_default(**default)

    return kodi_setting


def retrieve_default(function, value):
    if function is not None:
        value = function()

    return value


def kodi_to_config(settings, config, new_settings):
    """
        Takes the existing config.txt (as a list of lines) and constructs a new one using the
        settings from kodi.

        Returns a brand new config (list of lines)
    """

    # print "==--==--"*20 

    # print "Settings being sent to config.txt"

    # print "==--==--"*20
    '''
        It is vital for gpio-ir entry to come AFTER the gpio-ir-overlay entry,
        as the overlay entry is simply there to pick up legacy entries and eliminate
        them. The non-overlay entry is the one used to put them back into the config.txt
        file. Dictionaries aren't order in python 2, so we have to do the ordering
        using a sorted list of keys. (!!!!)
    '''
    new_setting_keys = new_settings.keys()
    new_setting_keys = sorted(new_setting_keys)

    for setting in new_setting_keys:
        new_value = new_settings[setting]

        # print "%s: %s" % (setting, new_value)

        setting_protocols = settings.get(setting, None)
        if setting_protocols is None:
            # print "No setting protocol for %s" % setting
            continue

        # print "RUNNING CONFIG SET FOR %s" % setting
        config = general_config_set(config, new_settings, new_value, **setting_protocols)

    return config


def general_config_set(config, new_settings, new_value, config_get_patterns,
                       config_set, already_set, setting_stub, **kwargs):
    """
        Runs through the config.txt looking for a specific setting and replaces the existing
        values when they are found. If there are duplicate entries, the last entry is kept,
        the others are commented out.

        If not found, then the setting value is added to the end.

        Returns a new list of config lines.
    """

    new_config = []

    # pass the new_value through the config_set protocol to prepare it for inclusion in
    # the config.txt
    new_value = config_set(new_value, new_settings)

    # print config
    # print " \n"

    # the original config is run through backwards so that the last setting of any
    # duplicates is kept
    for line in config[::-1]:

        # print 'Examining line : %s ' % line.strip()

        # pass blank lines straight through
        if not line.strip():
            new_config.append(line)
            # print '\tLine is passed through as it is blank'
            continue

        # pass commented out lines straight through
        if line.strip().startswith("#"):
            new_config.append(line)
            # print '\tLine is passed through as it is a comment'
            continue

        # ignore inline comments on the line
        try:
            cf_line = line[: line.index("#")]
            comment = line[line.index("#"):]
        except ValueError:
            cf_line = line
            comment = ""

        line_matches = False

        for idx, pair in enumerate(config_get_patterns):
            matched = re.search(pair["identify"], cf_line, re.IGNORECASE)

            # print '\tAttempting to match line to pattern %s' % idx

            if matched:
                line_matches = True

                # print '\t\t match found'

                # if a match happens but the value is 'remove_this_line',
                # then dont add the line to the new config.txt
                if new_value == "remove_this_line":
                    # print '\t\tSetting value says to remove this line'
                    continue

                # if the value has been set already, then comment out this line
                if already_set:
                    # comment out any duplicated entries
                    # (this could be changed to removal if we want)
                    new_config.append("#" + line.replace("\n", "") + " # DUPLICATE")
                    # print '\t\tSetting has already been set, skipping subsequent matches
                    # and marking as DUPLICATE'
                    continue

                # otherwise update the line
                new_config.append(setting_stub % new_value + "  %s" % comment)
                # print '\tLine has been update with the new value: %s' % new_value
                already_set = True

        # if no match actually occured then pass the line through to the new config
        if not line_matches:
            new_config.append(line)
            # print '\t-- NO MATCH --'

    # flip the config back around the other way, so new entries are added at the end
    new_config = new_config[::-1]

    if not already_set and new_value != "remove_this_line":
        # print "Failed to find matched line in existing config, so adding a new line"
        new_line = setting_stub % new_value
        new_config.append(new_line)
        # print "\tNew line added: %s" % (setting_stub % new_value)
        # print "\tLines now read:"
        # for x in new_config: print "\t\t - %s" % x

    return new_config


'''
Validation functions, settings coming FROM the config.txt pass through one of these. They can be 
tested for accuracy, valid ranges, and (if it is a simple 1 for 1) converted to what is recognised 
by the settings in Kodi.
'''


def generic_bool_validation(config_value):
    permitted = {
        "0": "false",
        "1": "true"
    }

    return permitted.get(config_value, None)


def generic_range_validation(config_value, myrange):
    try:
        if int(config_value) in myrange:
            return config_value
        raise ValueError
    except (TypeError, ValueError):
        return None


def onoff_validation(config_value):
    permitted = {
        "off": "false",
        "on": "true"
    }

    return permitted.get(config_value, None)


def config_hdmi_boost_validation(config_value):
    return generic_range_validation(config_value, range(1, 12))


def soundcard_dac_validation(config_value):
    permitted = {
        "hifiberry-dac-overlay": "1",
        "hifiberry-dac": "1",
        "hifiberry-dacplus-overlay": "2",
        "hifiberry-dacplus": "2",
        "hifiberry-digi-overlay": "3",
        "hifiberry-digi": "3",
        "iqaudio-dac-overlay": "4",
        "iqaudio-dac": "4",
        "iqaudio-dacplus-overlay": "5",
        "iqaudio-dacplus": "5",
        "justboom-dac": "6",
        "justboom-dac-overlay": "6",
        "justboom-digi": "7",
        "justboom-digi-overlay": "7",
        "allo-piano-dac-pcm512x-audio-overlay": "8",
        "allo-piano-dac-pcm512x-audio": "8",
        "allo-boss-dac-pcm512x-audio-overlay": "9",
        "allo-boss-dac-pcm512x-audio": "9",
        "allo-digione-overlay": "10",
        "allo-digione": "10",
    }

    try:
        return permitted.get(config_value, None)
    except:
        return None


def gpio_pin_validation(config_value):
    return generic_range_validation(config_value, range(1, 28))


def blank_check_validation(config_value):
    if config_value:
        return config_value


def display_rotate_validation(config_value):
    permitted = ["0", "1", "2", "3", "0x10000", "0x20000"]

    if config_value in permitted:
        return config_value


def gpu_mem_1024_validation(config_value):
    return generic_range_validation(config_value, range(16, 321))


def gpu_mem_512_validation(config_value):
    return generic_range_validation(config_value, range(16, 257))


def gpu_mem_256_validation(config_value):
    return generic_range_validation(config_value, range(16, 193))


def hdmi_group_validation(config_value):
    return generic_range_validation(config_value, range(0, 3))


def hdmi_mode_validation(config_value):
    return generic_range_validation(config_value, range(1, 87))


def hdmi_pixel_encoding_validation(config_value):
    return generic_range_validation(config_value, range(0, 5))


def sdtv_aspect_validation(config_value):
    return generic_range_validation(config_value, range(1, 4))


def sdtv_mode_validation(config_value):
    return generic_range_validation(config_value, range(0, 4))


def w1gpio_validation(config_value):
    permitted = ["w1-gpio-overlay", "w1-gpio-pullup-overlay"]

    try:
        return str(permitted.index(config_value) + 1)
    except (ValueError, IndexError):
        return None


def bcm2835_validation(config_value):
    permitted = ["spi-bcm2835-overlay"]

    if config_value in permitted:
        return "true"


def hdmi_ignore_edid_validation(config_value):
    permitted = ["0xa5000080"]

    if config_value in permitted:
        return "true"


'''
Custom default functions, allows for customisable default values, 
i.e. default values determined by some external analysis
'''


def hdmi_boost_custom_default():
    """
        Tests the users system to see which hdmi_boost figure should be used.
        *** Yet to be implemented
    """
    return "0"


'''
Custom Config set functions, converts the kodi settings into the settings values required 
in the config.txt. Takes both the new value of the setting and all the other settings for reference.
'''


def generic_bool_config_set(kodi_setting, all_settings):
    if kodi_setting == "true":
        return "1"

    else:
        return "remove_this_line"


def generic_passthrough_config_set(kodi_setting, all_settings):
    if kodi_setting:
        return kodi_setting

    else:
        return "remove_this_line"


def start_x_config_set(kodi_setting, all_settings):
    """ Always return 1. This setting should be in every config.txt """
    return "1"


def config_hdmi_boost_config_set(kodi_setting, all_settings):
    # if hdmi_safe is active, then remove this conflicting line
    kodi_setting = hdmi_safe_group_removal(kodi_setting, all_settings)

    if kodi_setting in [str(x) for x in range(1, 12)]:
        return kodi_setting

    else:
        return "remove_this_line"


def display_rotate_config_set(kodi_setting, all_settings):
    permitted = ["remove_this_line", "1", "2", "3", "0x10000", "0x20000"]

    if kodi_setting in permitted:
        return kodi_setting

    else:
        return "remove_this_line"


def store_hdmi_to_file_config_set(kodi_setting, all_settings):
    # "hdmi_force_hotplug=%s\nhdmi_edid_file=%s"
    # if hdmi_safe is active, then remove this conflicting line
    kodi_setting = hdmi_safe_group_removal(kodi_setting, all_settings)

    if kodi_setting == "true":
        return "1", "1"

    else:
        return "remove_this_line", "remove_this_line"


def hdmi_group_config_set(kodi_setting, all_settings):
    # if hdmi_safe is active, then remove this conflicting line
    kodi_setting = hdmi_safe_group_removal(kodi_setting, all_settings)

    if kodi_setting in [str(x) for x in range(1, 3)]:
        return kodi_setting

    else:
        return "remove_this_line"


def hdmi_mode_config_set(kodi_setting, all_settings):
    # if hdmi_safe is active, then remove this conflicting line
    kodi_setting = hdmi_safe_group_removal(kodi_setting, all_settings)

    if kodi_setting in [str(x) for x in range(1, 87)]:
        return kodi_setting

    else:
        return "remove_this_line"


def hdmi_pixel_config_set(kodi_setting, all_settings):
    if kodi_setting in [str(x) for x in range(1, 5)]:
        return kodi_setting

    else:
        return "remove_this_line"


def hdmi_safe_group_removal(kodi_setting, all_settings):
    if all_settings.get("hdmi_safe", None) == "true":
        return "remove_this_line"

    else:
        return kodi_setting


def hdmi_ignore_edid_config_set(kodi_setting, all_settings):
    # if hdmi_safe is active, then remove this conflicting line
    kodi_setting = hdmi_safe_group_removal(kodi_setting, all_settings)

    if kodi_setting == "true":
        return "0xa5000080"

    else:
        return "remove_this_line"


def sdtv_aspect_config_set(kodi_setting, all_settings):
    if kodi_setting in [str(x) for x in range(1, 4)]:
        return kodi_setting

    else:
        return "remove_this_line"


def sdtv_mode_config_set(kodi_setting, all_settings):
    if kodi_setting in [str(x) for x in range(1, 4)]:
        return kodi_setting

    else:
        return "remove_this_line"


def bcm2835_config_set(kodi_setting, all_settings):
    if kodi_setting == "true":
        return "spi-bcm2835-overlay"

    else:
        return "remove_this_line"


def w1gpio_config_set(kodi_setting, all_settings):
    permitted = ["remove_this_line", "w1-gpio-overlay", "w1-gpio-pullup-overlay"]

    try:
        return permitted[int(kodi_setting)]
    except (ValueError, IndexError):
        return "remove_this_line"


def soundcard_dac_config_set(kodi_setting, all_settings):
    permitted = [
        "remove_this_line",
        "hifiberry-dac-overlay",
        "hifiberry-dacplus-overlay",
        "hifiberry-digi-overlay",
        "iqaudio-dac-overlay,unmute_amp",
        "iqaudio-dacplus-overlay,unmute_amp",
        "justboom-dac-overlay",
        "justboom-digi-overlay",
        "allo-piano-dac-pcm512x-audio-overlay",
        "allo-boss-dac-pcm512x-audio-overlay",
        "allo-digione-overlay",
    ]

    try:
        return permitted[int(kodi_setting)]
    except (ValueError, IndexError):
        return "remove_this_line"


def legacy_gpio_removal(kodi_setting, all_settings):
    """
        This entry is deprecated and should always be removed from the config.txt.
    """

    return "remove_this_line"


def gpio_group_removal(kodi_setting, all_settings):
    if all_settings.get("lirc-rpi-overlay") != "true":

        return "remove_this_line"
    else:

        return kodi_setting


def gpio_pin_config_set(kodi_setting, all_settings):
    if kodi_setting not in ["0", 0]:

        return int(kodi_setting)
    else:

        return "remove_this_line"


def audio_config_set(kodi_setting, all_settings):
    if all_settings.get("soundcard_dac", "0") != "0":

        return "off"
    else:

        return "remove_this_line"


def hdmi_force_hotplug_config_set(kodi_setting, all_settings):
    """
        hdmi_edid_file needs hdmi_force_hotplug but hdmi_force_hotplug doesnt need hdmi_edid_file
    """
    if kodi_setting == "true":
        return "1"

    elif all_settings.get("hdmi_edid_file", None) == "true":
        # if hdmi_edid_file is true in the kodi settings, then
        # force hdmi_force_hotplug to be active in the config.txt
        return "1"

    else:
        return "remove_this_line"


'''
Custom Kodi set functions, allows for more complex conversion of config settings 
(including multiple settings) into specific kodi settings.
'''


def generic_passthrough_kodi_set(results):
    """
        Takes a results list, and simply returns the first value.
    """

    try:
        setting_value = results[0]
    except IndexError:
        setting_value = None

    return setting_value


'''
Kodi Settings dictionary, houses the protocols for the settings in Kodi which come 
from the config.txt
'''

MASTER_SETTINGS = {
    "audio": {
        "default": {
            "function": None,
            "value": "false"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtparam|dtparams|device_tree_param|device_tree_params)"
                            r"\s*=.*audio\s*=",
                "extract": r"\s*(?:dtparam|dtparams|device_tree_param|device_tree_params)"
                           r"\s*=.*audio\s*=\s*(\w+)",
            }
        ],
        "config_set": audio_config_set,
        "config_validation": blank_check_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "dtparam=audio=%s",
    },
    "config_hdmi_boost": {
        "default": {
            "function": hdmi_boost_custom_default,
            "value": ""
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:hdmi_boost|config_hdmi_boost)\s*=",
                "extract": r"\s*(?:hdmi_boost|config_hdmi_boost)\s*=\s*(\d*)",
            }
        ],
        "config_set": config_hdmi_boost_config_set,
        "config_validation": config_hdmi_boost_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "config_hdmi_boost=%s",
    },
    "decode_MPG2": {
        "default": {
            "function": None,
            "value": ""
        },
        "config_get_patterns": [
            {
                "identify": r"\s*decode_MPG2\s*=\s*",
                "extract": r"\s*decode_MPG2\s*=\s*(\w+)"
            }
        ],
        "config_set": generic_passthrough_config_set,
        "config_validation": blank_check_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "decode_MPG2=%s",
    },
    "decode_WVC1": {
        "default": {
            "function": None,
            "value": ""
        },
        "config_get_patterns": [
            {
                "identify": r"\s*decode_WVC1\s*=\s*",
                "extract": r"\s*decode_WVC1\s*=\s*(\w+)"
            }
        ],
        "config_set": generic_passthrough_config_set,
        "config_validation": blank_check_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "decode_WVC1=%s",
    },
    "display_rotate": {
        "default": {
            "function": None,
            "value": "0"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*display_rotate\s*=\s*",
                "extract": r"\s*display_rotate\s*=\s*(\w+)"
            }
        ],
        "config_set": display_rotate_config_set,
        "config_validation": display_rotate_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "display_rotate=%s",
    },
    "gpu_mem_1024": {
        "default": {
            "function": None,
            "value": "256"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*gpu_mem_1024\s*=",
                "extract": r"\s*gpu_mem_1024\s*=\s*(\d+)"
            },
            {
                "identify": r"\s*gpu_mem\s*=",
                "extract": r"\s*gpu_mem\s*=\s*(\d+)"
            },
        ],
        "config_set": generic_passthrough_config_set,
        "config_validation": gpu_mem_1024_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "gpu_mem_1024=%s",
    },
    "gpu_mem_512": {
        "default": {
            "function": None,
            "value": "144"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*gpu_mem_512\s*=",
                "extract": r"\s*gpu_mem_512\s*=\s*(\d+)"
            },
            {
                "identify": r"\s*gpu_mem\s*=",
                "extract": r"\s*gpu_mem\s*=\s*(\d+)"
            },
        ],
        "config_set": generic_passthrough_config_set,
        "config_validation": gpu_mem_512_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "gpu_mem_512=%s",
    },
    "gpu_mem_256": {
        "default": {
            "function": None,
            "value": "112"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*gpu_mem_256\s*=",
                "extract": r"\s*gpu_mem_256\s*=\s*(\d+)"
            },
            {
                "identify": r"\s*gpu_mem\s*=",
                "extract": r"\s*gpu_mem\s*=\s*(\d+)"
            },
        ],
        "config_set": generic_passthrough_config_set,
        "config_validation": gpu_mem_256_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "gpu_mem_256=%s",
    },
    "hdmi_force_hotplug": {
        "default": {
            "function": None,
            "value": "false"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_force_hotplug\s*=",
                "extract": r"\s*hdmi_force_hotplug\s*=\s*(\d+)",
            }
        ],
        "config_set": hdmi_force_hotplug_config_set,
        "config_validation": generic_bool_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_force_hotplug=%s",
    },
    "hdmi_edid_file": {
        "default": {
            "function": None,
            "value": "false"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_edid_file\s*=",
                "extract": r"\s*hdmi_edid_file\s*=\s*(\d+)"
            }
        ],
        "config_set": generic_bool_config_set,
        "config_validation": generic_bool_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_edid_file=%s",
    },
    "hdmi_group": {
        "default": {
            "function": None,
            "value": "0"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_group\s*=",
                "extract": r"\s*hdmi_group\s*=\s*(\d+)"
            }
        ],
        "config_set": hdmi_group_config_set,
        "config_validation": hdmi_group_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_group=%s",
    },
    "hdmi_ignore_cec": {
        "default": {
            "function": None,
            "value": "false"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_ignore_cec\s*=",
                "extract": r"\s*hdmi_ignore_cec\s*=\s*(\d+)"
            }
        ],
        "config_set": generic_bool_config_set,
        "config_validation": generic_bool_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_ignore_cec=%s",
    },
    "hdmi_ignore_cec_init": {
        "default": {
            "function": None,
            "value": "false"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_ignore_cec_init\s*=",
                "extract": r"\s*hdmi_ignore_cec_init\s*=\s*(\d+)",
            }
        ],
        "config_set": generic_bool_config_set,
        "config_validation": generic_bool_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_ignore_cec_init=%s",
    },
    "hdmi_ignore_edid": {
        "default": {
            "function": None,
            "value": "false"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_ignore_edid\s*=",
                "extract": r"\s*hdmi_ignore_edid\s*=\s*(\w+)"
            }
        ],
        "config_set": hdmi_ignore_edid_config_set,
        "config_validation": hdmi_ignore_edid_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_ignore_edid=%s",
    },
    "hdmi_mode": {
        "default": {
            "function": None,
            "value": "0"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_mode\s*=",
                "extract": r"\s*hdmi_mode\s*=\s*(\d+)"
            }
        ],
        "config_set": hdmi_mode_config_set,
        "config_validation": hdmi_mode_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_mode=%s",
    },
    "hdmi_pixel_encoding": {
        "default": {
            "function": None,
            "value": "0"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_pixel_encoding\s*=",
                "extract": r"\s*hdmi_pixel_encoding\s*=\s*(\d+)",
            }
        ],
        "config_set": hdmi_pixel_config_set,
        "config_validation": hdmi_pixel_encoding_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_pixel_encoding=%s",
    },
    "hdmi_safe": {
        "default": {
            "function": None,
            "value": "false"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*hdmi_safe\s*=",
                "extract": r"\s*hdmi_safe\s*=\s*(\d+)"
            }
        ],
        "config_set": generic_bool_config_set,
        "config_validation": generic_bool_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "hdmi_safe=%s",
    },
    "sdtv_aspect": {
        "default": {
            "function": None,
            "value": "1"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*sdtv_aspect\s*=",
                "extract": r"\s*sdtv_aspect\s*=\s*(\d+)"
            }
        ],
        "config_set": sdtv_aspect_config_set,
        "config_validation": sdtv_aspect_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "sdtv_aspect=%s",
    },
    "sdtv_mode": {
        "default": {
            "function": None,
            "value": "0"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*sdtv_mode\s*=",
                "extract": r"\s*sdtv_mode\s*=\s*(\d+)"
            }
        ],
        "config_set": sdtv_mode_config_set,
        "config_validation": sdtv_mode_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "sdtv_mode=%s",
    },
    "spi-bcm2835-overlay": {
        "default": {
            "function": None,
            "value": "false"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay)"
                            r"\s*=\s*[-\w\d]*spi-bcm2835[-\w\d]*",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay)"
                           r"\s*=\s*([-\w\d]*spi-bcm2835[-\w\d]*)",
            }
        ],
        "config_set": bcm2835_config_set,
        "config_validation": bcm2835_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "dtoverlay=%s",
    },
    "w1gpio": {
        "default": {
            "function": None,
            "value": "0"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay)\s*=.*w1-gpio",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay)\s*=\s*([-\w\d]*w1-gpio[-\w\d]*)",
            }
        ],
        "config_set": w1gpio_config_set,
        "config_validation": w1gpio_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "dtoverlay=%s",
    },
    "soundcard_dac": {
        "default": {
            "function": None,
            "value": "0"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay)\s*=\s*[-\w\d]*"
                            r"(?:hifiberry-d|iqaudio-d|justboom-d|allo-piano-d|"
                            r"allo-boss-d|allo-digione-d)",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay)\s*=\s*([-\w\d]*"
                           r"(?:hifiberry-d|iqaudio-d|justboom-d|allo-piano-d|"
                           r"allo-boss-d|allo-digione-d)[-\w\d]*)",
            }
        ],
        "config_set": soundcard_dac_config_set,
        "config_validation": soundcard_dac_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "dtoverlay=%s",
    },
    "start_x": {
        "default": {
            "function": None,
            "value": "1"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*start_x\s*=",
                "extract": r"\s*start_x\s*=\s*(\d)"
            }
        ],
        "config_set": start_x_config_set,
        "config_validation": blank_check_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "start_x=%s",
    },
    "lirc-rpi-overlay": {
        "default": {
            "function": None,
            "value": "defunct"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay)\s*=\s*[-\w\d]*lirc-rpi[-\w\d]*",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay)\s*=\s*([-\w\d]*lirc-rpi[-\w\d]*)",
            }
        ],
        "config_set": legacy_gpio_removal,
        "config_validation": blank_check_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "",
    },

    "gpio_in_pull": {
        "default": {
            "function": None,
            "value": "defunct"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                            r"device_tree_param|device_tree_params)\s*=(?:lirc-rpi:)?"
                            r".*gpio_in_pull[-\w\d]*=",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                           r"device_tree_param|device_tree_params)\s*=(?:lirc-rpi:)?"
                           r".*gpio_in_pull[-\w\d]*=\s*(\w*)",
            }
        ],
        "config_set": legacy_gpio_removal,
        "config_validation": blank_check_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "",
    },
    "gpio-ir-overlay": {
        # This group looks for the new gpio-ir setting and deletes it when found.
        # The gpio_in_pin or gpio_pin settings control whether dtoverlay=gpio-ir
        # is added to the config.txt.
        "default": {
            "function": None,
            "value": "defunct"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay)\s*=\s*[-\w\d]*gpio-ir[-\w\d]*",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay)\s*=\s*([-\w\d]*gpio-ir[-\w\d]*)",
            }
        ],
        "config_set": legacy_gpio_removal,
        "config_validation": blank_check_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "",
    },
    "gpio_out_pin": {
        "default": {
            "function": None,
            "value": "defunct"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                            r"device_tree_param|device_tree_params)\s*=(?:lirc-rpi:)?"
                            r".*gpio_out_pin[-\w\d]*=",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                           r"device_tree_param|device_tree_params)\s*=(?:lirc-rpi:)?"
                           r".*gpio_out_pin[-\w\d]*=\s*(\w*)",
            }
        ],
        "config_set": legacy_gpio_removal,
        "config_validation": blank_check_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "",
    },
    "gpio_in_pin": {
        "default": {
            "function": None,
            "value": "defunct"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                            r"device_tree_param|device_tree_params)\s*=(?:lirc-rpi:)?"
                            r".*gpio_in_pin[-\w\d]*=",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                           r"device_tree_param|device_tree_params)\s*=(?:lirc-rpi:)?"
                           r".*gpio_in_pin[-\w\d]*=\s*(\w*)",
            }
        ],
        "config_set": legacy_gpio_removal,  # Coming from Kodi to the config.txt
        "config_validation": gpio_pin_validation,  # Coming from the config.txt to Kodi
        "kodi_set": generic_passthrough_kodi_set,  # Coming from the config.txt to Kodi
        "already_set": False,
        "setting_stub": "",
    },
    "gpio_pin": {
        "default": {
            "function": None,
            "value": "0"
        },
        "config_get_patterns": [
            {
                "identify": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                            r"device_tree_param|device_tree_params)\s*=(?:gpio-ir:)?"
                            r".*gpio_pin[-\w\d]*=",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                           r"device_tree_param|device_tree_params)\s*=(?:gpio-ir:)?"
                           r".*gpio_pin[-\w\d]*=\s*(\w*)",
            },
            {  # Legacy pin in extraction
                "identify": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                            r"device_tree_param|device_tree_params)\s*=(?:lirc-rpi:)?"
                            r".*gpio_in_pin[-\w\d]*=",
                "extract": r"\s*(?:dtoverlay|device_tree_overlay|dtparam|dtparams|"
                           r"device_tree_param|device_tree_params)\s*=(?:lirc-rpi:)?"
                           r".*gpio_in_pin[-\w\d]*=\s*(\w*)",
            },
        ],
        "config_set": gpio_pin_config_set,
        "config_validation": gpio_pin_validation,
        "kodi_set": generic_passthrough_kodi_set,
        "already_set": False,
        "setting_stub": "dtoverlay=gpio-ir,gpio_pin=%s",
    },
}


def read_config_file(location):
    with open(location, "r", encoding='utf-8') as f:
        return f.readlines()


def write_config_file(location, new_config):
    new_config = [
        x + "\n" if not x.endswith("\n") else x for x in new_config if "remove_this_line" not in x
    ]

    if PY2:
        new_config = [
            x.decode('utf-8') if isinstance(x, str) else x for x in new_config
        ]

    with open(location, "w", encoding='utf-8') as f:
        f.writelines(new_config)


def clean_config(config, patterns):
    """
        Reads the users config file and comments out lines that are problematic.
        This is determined using regex patterns.
    """
    comment_out_list = []

    for line in config:
        # ignore commented out lines
        if line.strip().startswith("#"):
            continue
        # prune the line to exclude inline comments
        if "#" in line:
            pure_line = line[: line.index("#")]
        else:
            pure_line = line
        # eliminate lines that ends with a comma
        if pure_line.strip().endswith(","):
            comment_out_list.append(line)
            continue
        # eliminate lines that match any of the patterns
        if any([re.search(pat, pure_line) for pat in patterns]):
            comment_out_list.append(line)

    new_config = [line if line not in comment_out_list else "#" + line for line in config]

    return new_config


if __name__ == "__main__":
    _config_txt = read_config_file('/boot/config.txt')
    _original_config = _config_txt[::]

    _extracted_settings = config_to_kodi(MASTER_SETTINGS, _config_txt)
    _new_settings = kodi_to_config(MASTER_SETTINGS, _original_config, _extracted_settings)

    write_config_file('/var/tmp/config.txt', _new_settings)

    subprocess.call(["sudo", "mv", '/var/tmp/config.txt', '/boot/config.txt'])
