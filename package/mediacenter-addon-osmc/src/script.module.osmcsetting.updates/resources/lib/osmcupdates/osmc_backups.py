# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import datetime
import json
import math
import os
import re
import shutil
import subprocess
import tarfile
import traceback
from copy import deepcopy
from io import open

import xbmc
import xbmcaddon
import xbmcgui
import xbmcvfs
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

ADDON_ID = 'script.module.osmcsetting.updates'
ADDON = xbmcaddon.Addon(ADDON_ID)
LANG = LangRetriever(ADDON).lang

TIME_PATTERN = '%Y_%m_%d_%H_%M_%S'
READ_PATTERN = '%Y-%m-%d %H:%M:%S'
APPENDAGE = '[0-9|_]*'
FILE_PATTERN = 'OSMCBACKUP_%s.tar.gz'

LOCAL_BACKUP_TEMP = '/tmp/osmc_backup/'
LOCAL_RESTORE_TEMP = '/tmp/osmc_restore/'

LOCATIONS = {
    'backup_addons': '{kodi_folder}addons/',
    'backup_addon_data': '{kodi_folder}userdata/addon_data/',
    'backup_Database': '{kodi_folder}userdata/Database/',
    'backup_keymaps': '{kodi_folder}userdata/keymaps/',
    'backup_library': '{kodi_folder}userdata/library/',
    'backup_playlists': '{kodi_folder}userdata/playlists/',
    'backup_profilesF': '{kodi_folder}userdata/profiles/',
    'backup_Thumbnails': '{kodi_folder}userdata/Thumbnails/',
    'backup_Savestates': '{kodi_folder}userdata/Savestates/',
    'backup_tvheadend': '{kodi_folder}userdata/.hts/',
    'backup_favourites': '{kodi_folder}userdata/favourites.xml',
    'backup_passwords': '{kodi_folder}userdata/passwords.xml',
    'backup_mediasources': '{kodi_folder}userdata/mediasources.xml',
    'backup_LCD': '{kodi_folder}userdata/LCD.xml',
    'backup_profiles': '{kodi_folder}userdata/profiles.xml',
    'backup_RssFeeds': '{kodi_folder}userdata/RssFeeds.xml',
    'backup_sources': '{kodi_folder}userdata/sources.xml',
    'backup_upnpserver': '{kodi_folder}userdata/upnpserver.xml',
    'backup_peripheral_data': '{kodi_folder}userdata/peripheral_data.xml',
    'backup_guisettings': '{kodi_folder}userdata/guisettings.xml',
    'backup_fstab': '{kodi_folder}userdata/fstab',
    'backup_advancedsettings': '{kodi_folder}userdata/advancedsettings.xml',
    'backup_samba_local': '{kodi_folder}userdata/smb-local.conf',
    'backup_autofs': '{kodi_folder}userdata/autofs',
    'backup_authorized_keys': '{kodi_folder}userdata/authorized_keys',
}

LABELS = {
    '{kodi_folder}/addons': '%s - %s' % (LANG(32189), LANG(32179)),
    '{kodi_folder}/userdata/addon_data': '%s - %s' % (LANG(32189), LANG(32180)),
    '{kodi_folder}/userdata/Database': '%s - %s' % (LANG(32189), LANG(32181)),
    '{kodi_folder}/userdata/keymaps': '%s - %s' % (LANG(32189), LANG(32182)),
    '{kodi_folder}/userdata/library': '%s - %s' % (LANG(32189), LANG(32183)),
    '{kodi_folder}/userdata/playlists': '%s - %s' % (LANG(32189), LANG(32184)),
    '{kodi_folder}/userdata/profiles': '%s - %s' % (LANG(32189), LANG(32185)),
    '{kodi_folder}/userdata/Thumbnails': '%s - %s' % (LANG(32189), LANG(32186)),
    '{kodi_folder}/userdata/Savestates': '%s - %s' % (LANG(32189), LANG(32187)),
    '{kodi_folder}/userdata/.hts': '%s - %s' % (LANG(32189), LANG(32188)),
    '{kodi_folder}/userdata/advancedsettings.xml': '%s - advancedsettings.xml' % LANG(32191),
    '{kodi_folder}/userdata/guisettings.xml': '%s - guisettings.xml' % LANG(32191),
    '{kodi_folder}/userdata/fstab': '%s - fstab' % LANG(32191),
    '{kodi_folder}/userdata/sources.xml': '%s - sources.xml' % LANG(32191),
    '{kodi_folder}/userdata/profiles.xml': '%s - profiles.xml' % LANG(32191),
    '{kodi_folder}/userdata/favourites.xml': '%s - favourites.xml' % LANG(32191),
    '{kodi_folder}/userdata/passwords.xml': '%s - passwords.xml' % LANG(32191),
    '{kodi_folder}/userdata/mediasources.xml': '%s - mediasources.xml' % LANG(32191),
    '{kodi_folder}/userdata/LCD.xml': '%s - LCD.xml' % LANG(32191),
    '{kodi_folder}/userdata/RssFeeds.xml': '%s - RssFeeds.xml' % LANG(32191),
    '{kodi_folder}/userdata/upnpserver.xml': '%s - upnpserver.xml' % LANG(32191),
    '{kodi_folder}/userdata/peripheral_data.xml': '%s - peripheral_data.xml' % LANG(32191),
    '{kodi_folder}/userdata/smb-local.conf': '%s - smb-local.conf' % LANG(32191),
    '{kodi_folder}/userdata/authorized_keys': '%s - %s' % (LANG(32191), LANG(32178)),
    '{kodi_folder}/userdata/autofs': '%s - %s' % (LANG(32190), LANG(32177)),
}

AUTOFS_SOURCE_FILES = [
    '/etc/auto.master',
    '/etc/auto.misc',
    '/etc/auto.net',
    '/etc/auto.nfs.shares',
    '/etc/auto.smb',
    '/etc/auto.smb.shares',
    '/etc/autofs.conf',
    '/etc/auto.master.d/',
]

AUTOFS_DEST_FILES = [
    '{kodi_folder}userdata/auto.master',
    '{kodi_folder}userdata/auto.misc',
    '{kodi_folder}userdata/auto.net',
    '{kodi_folder}userdata/auto.nfs.shares',
    '{kodi_folder}userdata/auto.smb',
    '{kodi_folder}userdata/auto.smb.shares',
    '{kodi_folder}userdata/autofs.conf',
    '{kodi_folder}userdata/auto.master.d/',
]

DIALOG = xbmcgui.Dialog()

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class OSMCBackup(object):

    def __init__(self, settings_dict, progress_function, parent_queue=None, addon=None):
        log('osmc_backup INIT')
        self._addon = addon

        self.settings_dict = settings_dict

        self.parent_queue = parent_queue

        self.progress = progress_function

        self.restoring_guisettings = False
        self.restoring_fstab = False

        self.location = None
        self.success = []

        # backup candidates is a list of tuples that contain the folder/file path and the
        # size in bytes of the entry
        self.backup_candidates = self.create_backup_file_list()

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(ADDON_ID)
        return self._addon

    @staticmethod
    def lang(value):
        return LANG(value)

    @staticmethod
    def prepare_backup_temp():
        try:
            shutil.rmtree(LOCAL_BACKUP_TEMP)
        except:
            pass

        try:
            os.mkdir(LOCAL_BACKUP_TEMP)
        except:
            log(traceback.format_exc())
            pass

        if not os.path.exists(LOCAL_BACKUP_TEMP):
            try:
                subprocess.Popen(['sudo', 'mkdir', LOCAL_BACKUP_TEMP])
            except:
                log(traceback.format_exc())
                pass

    @staticmethod
    def prepare_restore_temp():
        try:
            shutil.rmtree(LOCAL_RESTORE_TEMP)
        except:
            pass

        try:
            os.mkdir(LOCAL_RESTORE_TEMP)
        except:
            log(traceback.format_exc())
            pass

        if not os.path.exists(LOCAL_RESTORE_TEMP):
            try:
                subprocess.Popen(['sudo', 'mkdir', LOCAL_RESTORE_TEMP])
            except:
                log(traceback.format_exc())
                pass

    def start_backup(self):
        """
            This is the main method that walks through the backup process
        """
        self.prepare_backup_temp()

        if self.check_backup_location():
            self.create_tarball()

            msg = json.dumps(('pre_backup_complete', {}))
            try:
                self.parent_queue.put(msg)
            except:
                pass

    def check_backup_location(self):
        """
            Tests the backup location for disk space and writeable
        """
        self.location = self.settings_dict.get('backup_location', None)

        if not self.location:
            log('Location for backup not provided.')
            _ = DIALOG.ok(self.lang(32147), '[CR]'.join([self.lang(32148), self.lang(32149)]))

            return False

        # check for available disk space at backup location
        if not self.check_target_location_for_size(self.location):
            log('Insufficient disk space at target location')
            _ = DIALOG.ok(self.lang(32147), self.lang(32150))

            return False

        else:
            log('Sufficient disk space at target location')

        # check for write permission at backup location
        if not self.check_target_writeable(self.location):
            log('Backup location not writeable.')
            _ = DIALOG.ok(self.lang(32147), self.lang(32151))

            return False

        else:
            log('Backup location writeable.')

        return True

    @staticmethod
    def kodi_location():
        """
            returns the location of the kodi folder
        """
        return xbmcvfs.translatePath('special://home')

    def create_backup_file_list(self):
        """
            creates a list of the items to back-up
        """
        kodi_folder = self.kodi_location()

        backup_candidates = []

        locations = list(deepcopy(LOCATIONS).items())

        if self.settings_dict['backup_autofs']:  # expand autofs files
            locations = [(setting, destination_file) for setting, destination_file in locations if setting != 'backup_autofs']
            locations += [('backup_autofs', destination_file) for destination_file in AUTOFS_DEST_FILES]

        for setting, location in locations:
            if self.settings_dict[setting]:
                # if the user is backing up guisettings.xml, then call for the settings to be saved using
                # custom method
                if setting == 'backup_guisettings':
                    log('Trying to backup guisettings, running xbmc.saveSettings')

                    try:
                        xbmc.saveSettings()
                    except Exception as e:
                        log('xbmc.saveSetting failed')
                        log(type(e).__name__)
                        log(e.args)
                        log(traceback.format_exc())

                path = location.format(kodi_folder=kodi_folder)
                size = self.calculate_byte_size(path)

                backup_candidates.append((path, size))

        return backup_candidates

    def check_target_location_for_size(self, location):
        """
            Checks the target location to see if there is sufficient space for the tarball.
            Returns True if there is sufficient disk space
        """
        # the backup file gets created locally first, and then gets transferred,
        # so we have to make sure both location have sufficient free space.
        # linux cannot query free space from remote locations, so we just have to copy and hope

        # check locally
        try:
            st = os.statvfs(LOCAL_BACKUP_TEMP)

            requirement = self.estimate_disk_requirement()
            if st.f_frsize:
                available = st.f_frsize * st.f_bavail
            else:
                available = st.f_bsize * st.f_bavail
            # available	= st.f_bfree/float(st.f_blocks) * 100 * st.f_bsize

            log('local required disk space: %s' % requirement)
            log('local available disk space: %s' % available)

            if not requirement < available:
                return False
        except:
            pass

        # check remote
        try:
            st = os.statvfs(location)

            requirement = self.estimate_disk_requirement()
            if st.f_frsize:
                available = st.f_frsize * st.f_bavail
            else:
                available = st.f_bsize * st.f_bavail

            log('remote required disk space: %s' % requirement)
            log('remote available disk space: %s' % available)

            return requirement < available
        except:
            return True

    @staticmethod
    def check_target_writeable(location):
        """
            tests backup location is writeable
        """
        temp_file = os.path.join(location, 'temp_write_test')

        f = xbmcvfs.File(temp_file, 'w')

        try:
            _ = f.write('buffer')
        except:
            log('%s is not writeable' % location)
            f.close()
            return False

        f.close()

        try:
            xbmcvfs.delete(temp_file)
        except:
            log('Cannot delete temp file at %s' % location)

        return True

    def estimate_disk_requirement(self, func=None):
        sizes = [x[1] for x in self.backup_candidates]

        if func == 'log':
            sizes = [math.log(x) for x in sizes if x]

        return sum(sizes)

    @staticmethod
    def copy_to_backup(source_location, destination_location):
        """
            Copy `source_location` to the userdata `destination_location` folder so that it can be backed up.
        """

        try:
            if destination_location.endswith('/'):
                xbmcvfs.rmdir(destination_location, force=True)

            else:
                xbmcvfs.delete(destination_location)
        except:
            log('Failed to delete temporary %s' % destination_location)

        try:
            subprocess.Popen(['cp', '-rf', source_location, destination_location])

            log('%s successfully copied to %s' % (source_location, destination_location))

        except:
            log('Failed to copy %s file to %s')
            raise

    @staticmethod
    def move_to_restore(source_location, destination_location, sudo=False):
        """
            Move `source_location` to the `destination_location`
        """
        if destination_location.endswith('/') and not os.path.isdir(destination_location):
            try:
                if sudo:
                    subprocess.Popen(['sudo', 'mkdir', destination_location])
                else:
                    os.mkdir(destination_location)
            except:
                log(traceback.format_exc())
                pass

        try:
            if sudo:
                subprocess.Popen(['sudo', 'mv', source_location, destination_location])
            else:
                subprocess.Popen(['mv', source_location, destination_location])

        except:
            log(traceback.format_exc())
            log('Failed to move %s file to %s')
            raise

    def create_tarball(self):
        """
            takes the file list and creates a tarball in the backup location
        """

        location = self.settings_dict['backup_location']
        # get list of tarballs in backup location
        tarballs = self.list_current_tarballs(location)
        # check the users desired number of backups
        permitted_tarball_count = self.settings_dict['tarball_count']
        # determine how many extra tarballs there are
        extras = len(tarballs) - permitted_tarball_count + 1

        remove_these = []
        if extras > 0 and permitted_tarball_count != 0:
            remove_these = tarballs[:extras]

        # get the tag for the backup file
        tag = self.generate_tarball_name()
        # generate name for temporary tarball
        local_tarball_name = os.path.join(LOCAL_BACKUP_TEMP, FILE_PATTERN % tag)
        # generate name for remote tarball
        remote_tarball_name = os.path.join(location, FILE_PATTERN % tag)
        # get the size of all the files that are being backed up
        total_size = max(1, self.estimate_disk_requirement(func='log'))
        progress_total = 0

        # create a progress bar
        ''' 
            Controls the creation and updating of the background progress bar in kodi.
            The data gets sent from the apt_cache_action script via the socket
            percent, 	must be an integer
            heading,	string containing the running total of items, bytes and speed
            message, 	string containing the name of the package or the active process.
        '''
        pct = 0
        self.progress(**{
            'percent': pct,
            'heading': self.lang(32147),
            'message': self.lang(32159)
        })

        new_root = xbmcvfs.translatePath('special://home')

        try:
            with tarfile.open(name=local_tarball_name, mode="w:gz") as tar:
                for name, size in self.backup_candidates:
                    # if the user wants to backup the fstab file, then copy it to userdata
                    base_name = [f for f in name.split('/') if f][-1]
                    autofs_source = next((f for f in AUTOFS_SOURCE_FILES
                                          if f.endswith((base_name, base_name + '/'))), '')

                    if name.endswith('fstab'):
                        try:
                            self.copy_to_backup('/etc/fstab', name)
                        except:
                            continue

                    elif name.endswith('authorized_keys'):
                        try:
                            self.copy_to_backup('/home/osmc/.ssh/authorized_keys', name)
                        except:
                            continue

                    elif name.endswith('/.hts/'):
                        try:
                            self.copy_to_backup('/home/osmc/.hts/', name)
                        except:
                            continue

                    elif name.endswith('smb-local.conf'):
                        try:
                            self.copy_to_backup('/etc/samba/smb-local.conf', name)
                        except:
                            continue

                    elif autofs_source:
                        self.copy_to_backup(autofs_source, name)

                    self.progress(**{
                        'percent': pct,
                        'heading': self.lang(32147),
                        'message': '%s' % name
                    })

                    xbmc.sleep(150)  # sleep for 150ms to resolve invalid FileNotFound

                    try:
                        new_path = os.path.relpath(name, new_root)
                        if os.path.isdir(name) or os.path.isfile(name):
                            tar.add(name, arcname=new_path)
                        else:
                            log('Skipping %s, does not exist' % name)
                    except:
                        log('%s failed to backup to tarball' % name)
                        continue

                    progress_total += math.log(max(size, 1))
                    pct = int((progress_total / float(total_size)) * 100.0)

            # copy the local file to remote location
            self.progress(**{
                'percent': 100,
                'heading': self.lang(32147),
                'message': self.lang(32160)
            })

            log('local tarball name: %s' % local_tarball_name)
            log('local tarball exists: %s' % os.path.isfile(local_tarball_name))
            log('remote tarball name: %s' % remote_tarball_name)
            log('remote tarball exists: %s' % os.path.isfile(remote_tarball_name))

            log('Copying {local} to {remote} using shutil.copyfile'
                .format(local=local_tarball_name, remote=remote_tarball_name))
            try:
                shutil.copyfile(local_tarball_name, remote_tarball_name)
                xbmc.sleep(300)
                success = xbmcvfs.exists(remote_tarball_name)
            except FileNotFoundError:  # smb:// target will raise FileNotFoundError
                success = False

            if not success:
                log('Failed to copy {local} to {remote} using shutil.copyfile'
                    .format(local=local_tarball_name, remote=remote_tarball_name))
                log('Copying {local} to {remote} using xbmcvfs.copy'
                    .format(local=local_tarball_name, remote=remote_tarball_name))
                _ = xbmcvfs.copy(local_tarball_name, remote_tarball_name)
                xbmc.sleep(300)
                success = xbmcvfs.exists(remote_tarball_name)

            if not success:
                log('Failed to copy {local} to {remote} using xbmcvfs.copy'
                    .format(local=local_tarball_name, remote=remote_tarball_name))
                log('Copying {local} to {remote} using subprocess.Popen'
                    .format(local=local_tarball_name, remote=remote_tarball_name))
                subprocess.Popen(['cp', local_tarball_name, remote_tarball_name])
                xbmc.sleep(300)
                success = xbmcvfs.exists(remote_tarball_name)
                if not success:
                    log('Failed to copy {local} to {remote} using subprocess.Popen'
                        .format(local=local_tarball_name, remote=remote_tarball_name))

            if success:
                log('Backup file successfully transferred')
                try:
                    xbmcvfs.delete(local_tarball_name)
                except:
                    log('Cannot delete temp file at %s' % local_tarball_name)

            else:
                log('Transfer of backup file not successful: %s' % success)
                self.progress(kill=True)
                _ = DIALOG.ok(self.lang(32152), self.lang(32153))

                try:
                    xbmcvfs.delete(local_tarball_name)
                except:
                    log('Cannot delete temp file at %s' % local_tarball_name)

                return 'failed'

            # remove the unneeded backups (this will only occur if the tarball
            # is successfully created)
            log('Removing these files: %s' % remove_these)
            for r in remove_these:
                try:
                    self.progress(**{
                        'percent': 100,
                        'heading': self.lang(32147),
                        'message': self.lang(32161) % r
                    })
                    xbmcvfs.delete(os.path.join(location, r))
                except Exception as e:
                    log('Deleting tarball failed: %s' % r)
                    log(type(e).__name__)
                    log(e.args)
                    log(traceback.format_exc())

            self.progress(kill=True)

        except Exception as e:
            self.progress(kill=True)

            log('Creating tarball failed')
            log(type(e).__name__)
            log(e.args)
            log(traceback.format_exc())

            return 'failed'

    def start_restore(self):
        """
            Posts a list of backup files in the location, allows the user to choose one
            (including browse to a different location, allows the user to choose what to
            restore, including an ALL option.
        """
        self.prepare_restore_temp()

        location = self.settings_dict['backup_location']

        self.success = 'Full'

        current_tarballs = self.list_current_tarballs(location)
        # the first entry on the list dialog
        dialog_list = [(None, 'Browse for backup file')]
        # strip the boilerplate from the file and just show the name
        current_tarballs = [(name, self.strip_name(name, location)) for name in current_tarballs]
        # sort the list by date stamp (reverse)
        current_tarballs.sort(key=lambda x: x[1], reverse=True)
        # join the complete list together
        dialog_list.extend(current_tarballs)

        back_to_select = True
        temp_copy = None
        while back_to_select:
            # display the list
            file_selection = DIALOG.select(self.lang(32154), [x[1] for x in dialog_list])

            if file_selection == -1:
                back_to_select = False
                continue

            elif file_selection == 0:
                # open the browse dialog
                backup_file = DIALOG.browse(1, self.lang(32155), 'files')
                log('User selected backup file: %s' % backup_file)
                if not backup_file:
                    # return to select window
                    log('User has not selected backup file.')
                    continue

            else:
                # read the tar_file, post dialog with the contents
                # get file_selection
                remote_file = os.path.join(location, dialog_list[file_selection][0])
                basename = os.path.basename(remote_file)

                log('User selected backup file: %s' % remote_file)

                # this requires copying the tar_file from its stored location, to kodi/temp
                # xbmcvfs cannot read into tar files without copying the whole thing to memory
                temp_copy = os.path.join(LOCAL_RESTORE_TEMP, basename)

                result = xbmcvfs.copy(remote_file, temp_copy)
                if not result:
                    # copy of file failed
                    log('Failed to copy file to /tmp location')

                    _ = DIALOG.ok(self.lang(32145), self.lang(32156))
                    back_to_select = False
                    continue

                backup_file = temp_copy

            # open local copy and check for contents
            try:
                with tarfile.open(backup_file, 'r:gz') as t:
                    members = t.getmembers()

            except Exception as e:
                log('Opening and reading tarball failed')

                log(type(e).__name__)
                log(e.args)
                log(traceback.format_exc())

                _ = DIALOG.ok(self.lang(32145), self.lang(32146))
                continue

            if members:
                # the first entry on the list dialog, tuple is (member, display name,
                # name in tarfile, restore location)
                tar_contents_list = [(None, 'Everything')]

                # create list of items in the tar_file for the user to select from,
                # these are prime members; either they are the xml files or they
                # are the base folder
                menu_items = []
                for k, v in LABELS.items():
                    for member in members:
                        if k.endswith(member.name) or \
                                (k.endswith('userdata/autofs') and 'userdata/auto' in member.name):
                            menu_items.append((member, v))
                            break

                menu_items.sort(key=lambda x: x[1])
                tar_contents_list.extend(menu_items)

                if len(tar_contents_list) < 2:
                    log('Could not identify contents of backup file')
                    _ = DIALOG.ok(self.lang(32139), self.lang(32157))
                    continue

                # at the moment this only allows for a single item to be selected,
                # however we can build our own dialog that can allow multiple selection,
                # with the action only taking place on users OK
                item_selection = DIALOG.select(self.lang(32162),
                                               [x[1] for x in tar_contents_list])
                if item_selection == -1:
                    continue

                elif item_selection == 0:
                    log('User has chosen to restore all items')
                    # restore all items
                    restore_items = members

                else:
                    # restore single item
                    restore_item = tar_contents_list[item_selection]

                    # if the item is a single xml file, then restore that member,
                    # otherwise loop through the file members and collect the ones
                    # in the relevant folder the identification of a single file is
                    # hackish relying upon the presence of .xml, but for the moment
                    # we aren't backing up any other single files
                    if restore_item[0].name.endswith('.xml'):
                        restore_items = [restore_item[0]]
                        log('User has chosen to restore a single item: %s' % restore_item[1])

                    elif restore_item[1] == '%s - %s' % (LANG(32190), LANG(32177)):
                        log('User is restoring: %s' % restore_item[1])
                        restore_items = []
                        for member in members:
                            if 'userdata/auto' in member.name:
                                restore_items.append(member)
                    else:
                        log('User is restoring a folder: %s' % restore_item[1])
                        restore_items = []
                        for member in members:
                            if member.name.startswith(restore_item[0].name):
                                restore_items.append(member)

                # confirm with user that they want to overwrite existing files OR
                # extract to a different location
                overwrite = DIALOG.select(self.lang(32110), [self.lang(32163), self.lang(32164)])
                if overwrite == -1:
                    log('User has escaped restore dialog')
                    continue

                if overwrite == 0:
                    # restore over existing files
                    log('User has chosen to overwrite existing files.')
                    restore_location = xbmcvfs.translatePath('special://home')

                    # check the restore_items for guisettings.xml, if found then restore that
                    # to the addon_data folder, create the /RESET_GUISETTINGS file and write
                    # in the location of the restored guisettings.xml change the
                    # restoring_guisettings flag so the user is informed that this change
                    # requires a restart, or will take effect on next boot
                    if any([True for x in restore_items
                            if x.name.endswith('userdata/guisettings.xml')]):
                        self.restoring_guisettings = True

                    if any([True for x in restore_items if x.name.endswith('userdata/fstab')]):
                        self.restoring_fstab = True

                elif overwrite == 1:
                    # select new folder
                    log('User has chosen to browse for a new restore location')
                    restore_location = DIALOG.browse(3, self.lang(32165), 'files')

                else:
                    log('User has escaped restore dialog')
                    continue

                if restore_location == '':
                    log('User has failed to select a restore location')
                    continue

                with tarfile.open(backup_file, 'r') as t:
                    for member in restore_items:
                        log('attempting to restore %s to %s' % (member.name, restore_location))
                        try:
                            # if the item is userdata/guisettings.xml then restore it to /tmp
                            # and rename it the external script will pick it up there and
                            # overwrite the live version
                            if (member.name.endswith('userdata/guisettings.xml') and
                                    self.restoring_guisettings):
                                member.name = os.path.basename(member.name)
                                t.extract(member, '/tmp/')
                                os.rename('/tmp/guisettings.xml', '/tmp/guisettings.restore')

                            elif member.name.endswith('userdata/fstab') and self.restoring_fstab:
                                # restore temp version back to userdata
                                t.extract(member, restore_location)
                                fstab_loc = os.path.join(restore_location, member.name)
                                self.restore_fstab(fstab_loc)

                            elif member.name.endswith('userdata/authorized_keys'):
                                # restore temp version back to userdata
                                t.extract(member, restore_location)
                                authorized_keys_loc = os.path.join(restore_location, member.name)
                                self.move_to_restore(authorized_keys_loc, '/home/osmc/.ssh/',
                                                     sudo=False)

                            elif member.name.endswith('userdata/smb-local.conf'):
                                # restore temp version back to userdata
                                t.extract(member, restore_location)
                                samba_local_conf = os.path.join(restore_location, member.name)
                                self.move_to_restore(samba_local_conf, '/etc/samba/', sudo=True)

                            elif 'userdata/.hts/' in member.name:
                                # restore temp version back to userdata
                                t.extract(member, restore_location)
                                tvheadend_loc = os.path.join(restore_location, member.name)
                                self.move_to_restore(tvheadend_loc, '/home/osmc/.hts/',
                                                     sudo=False)

                            elif 'userdata/auto' in member.name and \
                                    not member.name.endswith('userdata/autofs'):
                                # restore temp version back to userdata
                                t.extract(member, restore_location)
                                autofs_file = os.path.join(restore_location, member.name)
                                self.move_to_restore(autofs_file, '/etc/', sudo=True)

                            else:
                                t.extract(member, restore_location)

                        except Exception as e:
                            log('Extraction of %s failed' % member.name)

                            log(type(e).__name__)
                            log(e.args)
                            log(traceback.format_exc())

                            if self.success == 'Full':
                                self.success = []

                            self.success.append(member.name)
                            continue

            if self.success == 'Full' and not self.restoring_guisettings:
                _ = DIALOG.ok(self.lang(32110), self.lang(32158))

            elif self.success == 'Full' and self.restoring_guisettings:
                back_to_select = False

            else:
                back_to_select = False

            try:
                xbmcvfs.delete(temp_copy)
            except:
                log('Deleting temp_copy failed. Don\'t worry, it might not exist.')

        self.progress(kill=True)

    @staticmethod
    def strip_name(name, location):
        """
            Returns the tarball file name with the boilerplate removed
        """
        f = xbmcvfs.File(os.path.join(location, name))
        size = f.size()
        f.close()

        date_string = name.replace('.tar.gz', '').replace('OSMCBACKUP_', '')

        letters = []
        count = 0
        for char in date_string:
            if char == '_':
                if count < 2:
                    letters.append('-')
                elif count == 2:
                    letters.append('  ')
                else:
                    letters.append(':')
                count += 1
            else:
                letters.append(char)

        if size:
            return 'Backup File  -  %s  -  %.*f MB' % \
                   (''.join(letters), 1, float(size) / 1024 / 1024)
        else:
            return 'Backup File  -  %s' % ''.join(letters)

    @staticmethod
    def generate_tarball_name():
        """
            Returns the name for the new tarball
        """
        file_tag = datetime.datetime.strftime(datetime.datetime.now(), TIME_PATTERN)
        return file_tag

    def list_current_tarballs(self, location):
        """
            Returns a list of the tarballs in the current backup location, from youngest to oldest
        """
        pattern = FILE_PATTERN % APPENDAGE
        dirs, tarball_list = xbmcvfs.listdir(location)

        log('Contents of remote location: %s' % tarball_list)

        regex = re.compile(pattern)
        tarball_list = [i for i in tarball_list if regex.search(i)]

        tarball_list.sort(key=lambda x: self.time_from_filename(x, pattern, location))

        log('tarball list from location: %s' % tarball_list)

        return tarball_list

    @staticmethod
    def time_from_filename(filename, pattern, location):
        """
            Returns the date of the backup that is embedded in the backup filename
        """
        prefix = FILE_PATTERN % APPENDAGE
        # extract just the relevant part of the string
        string = filename.replace(prefix[:prefix.index(APPENDAGE)], '').replace('.tar.gz', '')
        try:
            return datetime.datetime.strptime(string, TIME_PATTERN)

        except:
            return datetime.datetime(1, 1, 1, 1)

    @staticmethod
    def calculate_byte_size(candidate):
        if os.path.isfile(candidate):
            return os.path.getsize(candidate)

        if os.path.isdir(candidate):
            total_size = 0
            for dir_path, dir_names, file_names in os.walk(candidate):
                for f in file_names:
                    fp = os.path.join(dir_path, f)
                    try:
                        total_size += os.path.getsize(fp)
                    except:
                        pass

            return total_size

        return 0

    def count_stored_tarballs(self, location):
        """
            Counts the number of tarballs that are stored.
            Returns the count, along with the date of the earliest ball.
        """

    def export_libraries(self):
        """
            calls on kodi to export the selected libraries to a single .xml file
        """
        # exportlibrary(music,false,filepath)
        # The music library will be exported to a single file stored at filepath location.

        # exportlibrary(video,true,thumbs,overwrite,actorthumbs)
        # The video library is exported to multiple files with the given options.
        # Here thumbs, overwrite and actorthumbs are boolean values (true or false).

    @staticmethod
    def uniquify(list_with_duplicates):
        """
            Takes a list with duplicates and removes the duplicated lines,
            excluding blank lines or lines with only whitespace. Normally we
            could just use list(set(list_with_duplicates)) but that screws with
            the ordering of the list.
        """
        seen = []
        list_without_duplicates = []

        for item in list_with_duplicates:
            # if the line is pure whitespace, then retain it, and move on
            if not item.strip(' \n\t\r'):
                list_without_duplicates.append(item)
                continue

            # if we've already added this line, then move to the next one
            if item in seen:
                continue

            seen.append(item)
            list_without_duplicates.append(item)

        return list_without_duplicates

    def restore_fstab(self, location_of_backedup_fstab):
        """
            Restores the mounts in the fstab file
        """
        # run the backup file through millhams function to get a list of valid entries
        backup_mnts = {}  # milham_mod(location_of_backedup_fstab)
        unused_mnts = backup_mnts.copy()
        new_lines = []

        # read the current fstab file, process it line by line
        with open('/etc/fstab', 'r', encoding='utf-8') as current_fstab:
            lines = current_fstab.readlines()

        # process each line in the current fstab files
        for line in lines:
            # determine if the line contains a valid mount
            tup = {}

            # if not, or if the type of mount is no on this permitted list, then
            # add the line as-is to the new version, and then go to the next line
            if not tup or tup.fs_vfstype not in ['nfs', 'cifs', 'bind']:
                new_lines.append(line)
                continue

            # if there is a valid mount, then check that local mount point against those
            # found in the backup file we are restoring
            for mnt in backup_mnts:
                # if the local mount point is the same, then add the existing line to the new file,
                # put the backup mnt into new file as a comment, then remove the mnt from the
                # list of unused backup mnts
                if mnt.fs_file == tup.fs_file:
                    new_lines.append(line)
                    new_lines.append('#' + mnt.unparsed)
                    unused_mnts.remove(mnt)
                    break

            else:
                # if there is no match, then just add the line as-is to the new file
                new_lines.append(line)

        # add on all the unused mnts from the backup up file at then end
        new_lines.extend(unused_mnts)
        if new_lines:
            # create the new, replacement fstab file
            # we have to uniquify the list first, as the procedure above could result in multiple
            # commented out lines appearing in the restored fstab
            # it's just easier to remove them here than avoid adding them in the first place

            with open('/tmp/fstab', 'w', encoding='utf-8') as f:
                f.writelines(self.uniquify(new_lines))

            # finally, copy the temp fstab over the live fstab
            _ = subprocess.call(["sudo", "mv", '/tmp/fstab', '/etc/fstab'])
