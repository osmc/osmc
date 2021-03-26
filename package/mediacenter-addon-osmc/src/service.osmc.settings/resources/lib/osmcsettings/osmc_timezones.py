# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of service.osmc.settings

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

from io import open


def get_timezones():
    """ Returns a dictionary or regions, which hold lists of countries within those regions. """

    with open('/usr/share/zoneinfo/zone.tab', 'r', encoding='utf-8') as f:
        lines = f.readlines()

    lines = [line for line in lines if line and not line.startswith('#') and '/' in line]

    tmp = []
    timezones = {
        'UTC': ['UTC']
    }

    for line in lines:

        columns = line.split('\t')

        try:
            tz_raw = columns[2].replace('\n', '')
        except:
            continue

        tmp.append(tz_raw)

    tmp.sort()

    for tz_raw in tmp:
        tz_region, tz_country = tz_raw[:tz_raw.index('/')], tz_raw[tz_raw.index('/') + 1:]

        tz_item = timezones.get(tz_region, [])

        tz_item.append(tz_country)

        timezones[tz_region] = tz_item

    return timezones
