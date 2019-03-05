#!/usr/bin/env python3
import argparse

import sys
import logging
import json
import time
import threading
import re

import RPi.GPIO as GPIO

# Import sonar library
# https://github.com/MarkAHeywood/Bluetin_Python_Echo
from Bluetin_Echo import Echo

# Import from python-osc
from pythonosc import udp_client
from pythonosc import dispatcher
from pythonosc import osc_server
from typing import List, Any


LOGGER_FORMAT_DEBUG = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
LOGGER_FORMAT_INFO = '%(message)s'
logger = logging.getLogger(__name__)

class Config:

    def __init__(self):
        self.trigger = 21
        self.echo = 20
        self.threshold = 200 #cm
        # OSC path to send the sensor data to
        self.path_send_distance = "/id/machine/distance"
        # IP address for sending the sensor data to
        self.remote_host = "localhost"
        self.remote_port = 8888
        # OSC parth for receiving requests to change sensor delay
        self.path_set_delay = "/sensor/delay"
        # Must be network connected address to accept remote clients
        self.local_host = "0.0.0.0"
        self.local_port = 8888

class OSCClient:
    connection = False

    def __init__(self, osc_path, ip=None, port=None):
        self.osc_path = '{}'.format(osc_path)
        self.ip = ip
        self.port = port
        self.set_host("{}:{}{}".format(self.ip, self.port, self.osc_path))

    def send(self, msg):
        try:
            self.connection.send_message(self.osc_path, msg)
        except:
            pass

    def set_host(self, uri):
        logging.debug("Received new uri to send values to: {}".format(uri))
        r = re.search(r"(?P<ip>[^/^:]*):?(?P<port>\d+)?(?P<path>/.*)?", uri)
        try:
            if r.group('ip'):
                self.ip = r.group('ip')
            if r.group('port'):
                self.port = int(r.group('port'))
            if r.group('path'):
                self.osc_path = r.group('path')
            logging.debug("Sending values to: {}:{}{}".format(self.ip, self.port, self.osc_path))
            self.connection = udp_client.SimpleUDPClient(self.ip, self.port)
        except:
            self.connection = None

class OSCServer:

    def __init__(self, echo, osc, host="", port=""):
        self.echo = echo
        self.osc = osc
        self.dpr = dispatcher.Dispatcher()
        self.dpr.set_default_handler(self.default_handler, "Value")
        self.dpr.map("/sensor/delay", self.set_delay_handler)
        self.dpr.map("/host/path", self.set_path_handler)
        self.dpr.map("/id/machine/distance", self.read_distance_handler)

        self.server = osc_server.ThreadingOSCUDPServer(
            (host, port), self.dpr)
        logging.debug("Serving on {}".format(self.server.server_address))

    def _start_loop(self):
        self.loop=threading.Thread(target=self.server.serve_forever)
        self.loop.daemon = True
        self.loop.start()

    def __call__(self):
        self._start_loop()

    def set_delay_handler(self, address: str, *osc_arguments: List[Any]) -> None:
        if hasattr(self, 'echo'):
            try:
                value = float(osc_arguments[0])
            except:
                return
            logging.debug("setting delay to: {}".format(value))
            self.echo.rest = value

    def set_path_handler(self, address: str, *osc_arguments: List[Any]) -> None:
        if hasattr(self, 'osc'):
            logging.debug("setting remote host path: {}".format(osc_arguments))
            self.osc.set_host(osc_arguments[0])

    def read_distance_handler(self, address: str, *osc_arguments: List[Any]) -> None:
        logging.debug("distance is: {}".format(osc_arguments[0]))

    def default_handler(self, address: str, *osc_arguments: List[Any]) -> None:
        logging.debug("address: {}\narguments: {}".format(address,osc_arguments))


def _create_parser():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('-d', '--debug', action="store_true", default=True)

    return parser

def main(arguments=sys.argv[1:]):
    parser = _create_parser()
    args = parser.parse_args(arguments)
    if args.debug:
        logging.basicConfig(
            format=LOGGER_FORMAT_DEBUG,
            level=logging.DEBUG
        )
    else:
        logging.basicConfig(
            format=LOGGER_FORMAT_INFO,
            level=logging.INFO
        )

    logging.debug("Starting program")

    config = Config()

    # setup osc server and client, and echo sensor
    echo = Echo(config.trigger, config.echo)
    # Sends distance value to remote osc server
    osc = OSCClient(config.path_send_distance, config.remote_host, config.remote_port)

    # Receives value to set echo sensor delay
    oscserver = OSCServer(echo, osc, host=config.local_host, port=config.local_port)

    # start server thread
    oscserver()

    try:
        while 1:
            data = echo.read('cm', 1)
            time.sleep(echo.rest)
            osc.send(data)

    except KeyboardInterrupt:
        logging.debug("Cleaning up")
        GPIO.cleanup()

if __name__ == "__main__":
    main()
