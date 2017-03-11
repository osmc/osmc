# 2016-05-10 Brian Millham bmillham@gmail.com

import re
from collections import namedtuple

# RE to parse fstab fields
fields_re = re.compile(r'(?P<fs_spec>\S+)\s+(?P<fs_file>\S+)\s+'
                       r'(?P<fs_vfstype>\S+)\s+(?P<fs_mntops>\S+)\s*'
                       r'(?P<fs_freq>\d*)\s*(?P<fs_passno>\d*)')

# Namedtuple to save each line in
FSTabLine = namedtuple('FSTabLine',
                       'fs_spec fs_file fs_vfstype fs_mntops fs_freq '
                       'fs_passno spec_type mnt_dev unparsed is_comment is_empty is_bad')

# Empty dict to match the namedtuple
fstab_line_empty = {'fs_spec': None, 'fs_file': None, 'fs_vfstype': None, 'fs_mntops': None,
                    'fs_freq': '', 'fs_passno': '', 'spec_type': None, 'mnt_dev': None,
                    'unparsed': None, 'is_comment': False, 'is_empty': False, 'is_bad': False}

class fstab_compare(object):
    """ A class to make finding lines in a backedup fstab easy to find """
    def __init__(self, current_fstab, backup_fstab):
        """ Call with the current fstab, and the backup fstab """
        self.current_lines = self.__readfstab(current_fstab)
        self.backup_lines = self.__readfstab(backup_fstab)
        self.original_fs_files = [o.fs_file for o in self.current_lines if o.fs_file is not None]

    def __readfstab(self, file):
        """ Internal. Used to read the fstab file into the namedtuple """
        lines = []
        with open(file) as fstab_file:
            for line in fstab_file:
                lines.append(self.__namedtuple(line))
        return lines

    def __namedtuple(self, line):
        """ Internal. Convert a fstab line into a namedtuple """
        lines = []
        line = line.rstrip() # Remove trailing newlines
        d = fstab_line_empty.copy()
        if line == '':
            d['is_empty'] = True
        if not line.startswith('#'):
            try:
                d.update([m.groupdict() for m in fields_re.finditer(line)][0])
            except:
                d['is_bad'] = True
            try:
                d['spec_type'], d['mnt_dev'] = d['fs_spec'].split("=")
            except:
                pass
            # Sort fs_mntops so the order does not matter between the files
            if d['fs_mntops'] is not None:
                d['fs_mntops'] = ",".join(sorted(d['fs_mntops'].split(',')))
        else:
            d['is_comment'] = True
            d['unparsed'] = line
        return FSTabLine(**d)

    def __diffs(self):
        """ Internal. Create a list of lines in the backup fstab not found
            in the current fstab """
        return list(set(self.backup_lines) - set(self.current_lines))

    def formatted(self, line):
        """ Return a properly formatted fstab line """
        return "{.fs_spec} {.fs_file} {.fs_vfstype} {.fs_mntops} {.fs_freq} {.fs_passno}".format(line, line, line, line, line, line)

    def unique_fs_files(self):
        """ Return a list of the backed up fstab namedtuples (not in current fstab) """
        return [d for d in self.__diffs() if d.fs_file not in self.original_fs_files and d.fs_file is not None]

    def unique_fs_files_formatted(self):
        """ Return formatted lines of the backed up fstab lines (not in current fstab) """
        return [self.formatted(d) for d in self.unique_fs_files()]
