#!/usr/bin/python3

import logging
import argparse
import configparser

from src.dbus_lamp_bridge import *
from src.octoprint_json_poller import *

#################
### Utilities ###
#################

# command line parser
parser = argparse.ArgumentParser(description='Printer interaction program')
parser.add_argument('--config', type=str, required=True, help='Path to the config file for the service')
command_line_args = parser.parse_args()

class ConfigExtractor:
    def __init__(self):
        self.conf_parser = configparser.ConfigParser()
        try:
            self.conf_parser.read(command_line_args.config)
        except:
            print("ERROR: Could not read config file. Make sure that you handed over a valid path and ini-file.")
            exit(1)

        self.init_logger()

    def get_log_level(self):
        received_log_level = self.conf_parser['PRINTERSERVICE']['log_level']
        if received_log_level == "debug":
            return logging.DEBUG
        elif received_log_level == "info":
            return logging.INFO
        elif received_log_level == "warning":
            return logging.WARNING
        elif received_log_level == "error":
            return logging.ERROR
        elif received_log_level == "cirical":
            return logging.CRITICAL
        else:
            print("ERROR: Received invalid log level. Please review the app invocation")
            exit(1)

    def init_logger(self):
        # Logger init
        logging.basicConfig(format='%(levelname)s:%(message)s', level=self.get_log_level())
        logging.info("Logger initialized")

    def get_octopi_ip(self):
        return self.conf_parser['PRINTERSERVICE']['ip_adress']
    
    def get_octopi_port(self):
        return self.conf_parser['PRINTERSERVICE']['port']

    def get_octopi_api_key(self):
        return self.conf_parser['PRINTERSERVICE']['api_key']

    def get_heating_threshold(self):
        return self.conf_parser['PRINTERSERVICE']['heating_threshold']

    def get_heating_clip_bed(self):
        return float(self.conf_parser['PRINTERSERVICE']['heating_clip_bed'])

    def get_heating_clip_tool(self):
        return float(self.conf_parser['PRINTERSERVICE']['heating_clip_tool'])


##############################
### Entrypoint interaction ###
##############################

def main():
    config_obj = ConfigExtractor()
    logging.info("Starting: Try to connect with DBus and the printer lamp interaction service...")
    dbus_bright = DBusLampBridge()
    octoprint_interface_instance = OctoprintJsonPoller(config_obj.get_heating_threshold(), config_obj.get_heating_clip_bed(), config_obj.get_heating_clip_tool(), dbus_bright, config_obj.get_octopi_api_key(), config_obj.get_octopi_ip(), config_obj.get_octopi_port())
    logging.info("String up the app successful. Looking out for state changes...")
    octoprint_interface_instance.start_polling_loop()

if __name__ == "__main__":
    main()
