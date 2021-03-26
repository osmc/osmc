# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmccommon

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import socket
import subprocess
import sys
import threading
from contextlib import closing

import xbmc

from .osmc_logging import StandardLogger

PY3 = sys.version_info.major == 3

log = StandardLogger('script.module.osmccommon', os.path.basename(__file__)).log


class Communicator(threading.Thread):

    def __init__(self, parent_queue, socket_file):

        # queue back to parent
        self.parent_queue = parent_queue

        # not sure I need this, but oh well
        # self.wait_evt = threading.Event()

        threading.Thread.__init__(self)

        self.daemon = True

        self.monitor = xbmc.Monitor()

        # create the listening socket, it creates new connections when connected to
        self.address = socket_file

        if os.path.exists(self.address):
            subprocess.call(['sudo', 'rm', self.address])
            try:
                # I need this for testing on my laptop
                os.remove(self.address)
            except:
                log('Connection failed to delete socket file.')
                pass

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

        # allows the address to be reused (helpful with testing)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.timeout = 3
        self.sock.settimeout(self.timeout)
        self.sock.bind(self.address)
        self.sock.listen(1)

        self.stopped = False

    def stop(self):
        """ Orderly shutdown of the socket, sends message to run loop
            to exit. """

        log('Connection stop called')

        try:

            log('Connection stopping.')
            self.stopped = True

            message = 'exit'
            with closing(socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)) as open_socket:
                open_socket.connect(self.address)
                if PY3 and not isinstance(message, (bytes, bytearray)):
                    message = message.encode('utf-8', 'ignore')
                open_socket.send(message)

            self.sock.close()

            log('Exit message sent to socket.')

        except Exception as e:

            log('Comms error trying to stop: {}'.format(e))

    def run(self):

        log('Comms started')

        while not self.monitor.abortRequested() and not self.stopped:

            try:
                # wait here for a connection
                conn, addr = self.sock.accept()
            except socket.timeout:
                continue
            except:
                log('An error occurred while waiting for a connection.')
                break

            log('Connection active.')
            try:
                # turn off blocking for this temporary connection
                # this will allow the loop to collect all parts of the message
                conn.setblocking(False)

                passed = False
                total_wait = 0
                wait = 0.005
                data = ''
                while not passed and total_wait < 0.5:
                    try:
                        data = conn.recv(8192)
                        passed = True
                        if isinstance(data, bytes):
                            data = data.decode('utf-8')
                        log('data = %s' % data)
                    except:
                        total_wait += wait
                        if self.monitor.waitForAbort(wait):
                            break

                if not passed:
                    log('Connection failed to collect data.')
                    self.stopped = True
                    break

                log('data = %s' % data)

                # if the message is to stop, then kill the loop
                if data == 'exit':
                    log('Connection called to "exit"')
                    self.stopped = True
                    break

                # send the data to Main for it to process
                self.parent_queue.put(data)

            finally:
                conn.close()

        try:
            os.remove(self.address)
        except Exception as e:
            log('Comms error trying to delete socket: {}'.format(e))

        log('Comms Ended')
