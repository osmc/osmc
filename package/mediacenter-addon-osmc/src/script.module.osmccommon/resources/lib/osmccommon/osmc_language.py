# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmccommon

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import sys

PY2 = sys.version_info.major == 2


class LangRetriever(object):
    """ Used to retrieve localised strings from the addons po files.

        Requires the parent addon object. This takes the form in that parent script of:
            addon = xbmcaddon.Addon()

        Best usage:
            from osmc_language import LangRetriever
            LangRet = LangRetriever(addon)
            lang    = LangRet.lang

        """

    def __init__(self, addon=None):
        self.addon = addon

    def lang(self, string_id):
        if self.addon is None:
            return str(string_id)

        if self.addon is not None:

            if PY2:
                return self.addon.getLocalizedString(string_id).encode('utf-8', 'ignore')

            return self.addon.getLocalizedString(string_id)

        return ''
