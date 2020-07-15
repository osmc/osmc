# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.remotes

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import requests

URL_BASE = 'http://lirc.sourceforge.net/remotes/'


def get_base():
    """
        Returns a list of sub pages for remotes
    """
    response = requests.get(URL_BASE)
    text = response.text

    return [x[:x.index('/"')] for x in text.split('<a href="') if '/"' in x]


def get_sub_pages():
    """
        Returns a dictionary of conf files (keys) found in the sub-menu along with images (values) if there are any
    """

    ignore_chars = ['/', '?']
    image_chars = ['.jpg', '.png']

    confs = {}
    images = []

    subs = []

    response = requests.get(URL_BASE)
    text = response.text

    # create a list of links from the sub page
    subs_raw = [x[:x.index('"')] for x in text.split('<a href="') if '"' in x]

    # remove the links with those specific characters
    for sub in subs_raw:
        for ignored in ignore_chars:
            if ignored in sub:
                break
        else:
            subs.append(sub)

    # add the confs to the confs dict and push the images off to the images list
    for sub in subs:
        for img in image_chars:
            if img in sub:
                images.append(sub)
                break
        else:
            confs[sub] = None

    # cycle through the images and match them to the conf of the same name
    for image in images:
        cnf = image.replace('.png', '').replace('.jpg', '')
        if cnf in confs:
            confs[cnf] = image

    return confs
