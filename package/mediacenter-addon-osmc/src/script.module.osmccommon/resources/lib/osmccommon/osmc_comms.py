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
import threading
import traceback

import xbmc

from .osmc_logging import StandardLogger

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

        self.delete_sockfile()

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

        # allows the address to be reused (helpful with testing)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.timeout = 3
        self.sock.settimeout(self.timeout)
        self.sock.bind(self.address)
        self.sock.listen(1)

        self.stopped = False

    def delete_sockfile(self):
        if os.path.exists(self.address):
            subprocess.call(['sudo', 'rm', self.address])
            if os.path.exists(self.address):
                try:
                    # I need this for testing on my laptop
                    os.remove(self.address)
                except:
                    pass

        if os.path.exists(self.address):
            log('Failed to delete socket file @ %s.' % self.address)

    def close_socket(self):
        self.sock.close()
        self.delete_sockfile()

    def stop(self):
        """ Orderly shutdown of the socket, sends message to run loop
            to exit. """

        log('Communications shutting down... %s' % self.address)

        try:
            self.stopped = True

            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as open_socket:
                open_socket.connect(self.address)
                open_socket.send(b'exit')

            log('Exit message sent to socket... %s' % self.address)

        except:
            log('Communications error trying to shutdown... %s' % self.address)
            log(traceback.format_exc())

    def run(self):

        log('Communications started... %s' % self.address)

        while not self.monitor.abortRequested() and not self.stopped:

            try:
                # wait here for a connection
                conn, addr = self.sock.accept()
            except socket.timeout:
                continue
            except:
                exception_message = traceback.format_exc()

                if 'socket.timeout: timed out' in exception_message:
                    # required for edge case
                    # TODO: identify and resolve underlying issue
                    continue

                log('An error occurred while waiting for a connection... %s' % self.address)
                log(exception_message)
                break

            log('Connection is active... %s' % self.address)
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
                    except:
                        total_wait += wait
                        if self.monitor.waitForAbort(wait):
                            break

                        continue

                    passed = True
                    data = data.decode('utf-8')
                    log('Connection received partial data... %s @ %s' % (data, self.address))

                if not passed:
                    log('Connection received no data... %s' % self.address)
                    self.stopped = True
                    break

                log('Connection received data... %s @ %s' % (data, self.address))

                # if the message is to stop, then kill the loop
                if data == 'exit':
                    log('Connection called to shutdown...  %s' % self.address)
                    self.stopped = True
                    break

                # send the data to Main for it to process
                self.parent_queue.put(data)

            finally:
                conn.close()

        self.close_socket()
        log('Communications shutdown... %s' % self.address)
