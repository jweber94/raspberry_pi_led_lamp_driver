#!/usr/bin/python3

import dbus
import logging
import argparse
import configparser
import time
import requests

#################
### Utilities ###
#################

# Octopi url
PRINTER_STATE_RESOURCE = "/api/printer"

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

##############################
### DBus interaction class ###
##############################

class DBusLampBridge:
    def __init__(self):
        # DBus setup
        self.system_bus = dbus.SystemBus()
        self.reconnect_counter = 0
        self.connected_to_service = False
        while (self.reconnect_counter < 5) or (not self.connected_to_service):
            self.reconnect_counter += 1
            try:
                self.printer_lamp_proxy_dbus_test = self.system_bus.get_object('org.freedesktop.UPower', '/org/freedesktop/UPower/KbdBacklight') # TODO: Replace this by the printer lamp service
                #self.printer_lamp_proxy_dbus = self.system_bus.get_object('jens.printerlamp', '/3DP/printerlamp') # get the object path /3DP/printerlamp of the jens.printerlamp bus name (one busname can have multiple object paths underneath)
                self.printer_lamp_interface = dbus.Interface(self.printer_lamp_proxy_dbus_test, 'org.freedesktop.UPower.KbdBacklight') # get interface (all methods and attributes that the object provides)
                self.connected_to_service = True
            except:
                logging.warning("Could not connect to the DBus jens.printerlamp service. Try again")
                time.sleep(5) # sleep for 5 seconds and try again
        if not self.connected_to_service:
            logging.error("Could not connect to the jens.printerlamp dbus service after retrying. The service might not be installed! Check your system configuration and try again.")
            exit()

        # TODO: Add connect_to_signal to get notified if the new state was sucessfully set

    def get_state(self):
        #return self.printer_lamp_interface.GetLampState()
        return self.printer_lamp_interface.GetBrightness()

    def set_state(self, state):
        #self.printer_lamp_interface.SetLampState()
        self.printer_lamp_interface.SetBrightness(state)
        
class OctoprintJsonPoller():
    def __init__(self, dbus_bridge, octo_api_key, ip_addr, port=80):
        self.dbus_instance = dbus_bridge
        self.octo_api_key = octo_api_key
        self.ip_addr = ip_addr
        self.port = port
        logging.info("Octoprint poller initialized successfully")

    def get_printer_json(self):
        header = {'X-Api-Key': self.octo_api_key}
        url = 'http://' + self.ip_addr + PRINTER_STATE_RESOURCE # + 'foobar'
        resp = requests.get(url, headers=header).json()
        if 'error' in resp.keys():
            logging.warning("Invalid response from octoprint!")
        return resp

    def destil_printer_state(self, octoprint_json):
        return {'bed_temp': 0, 'tool_tmp': 0, 'global_state': 'printing'}

    def calcuate_lamp_state(self, destilled_state):
        return 6

    def polling_loop(self):
        while True:
            # octpoprint
            printer_json = self.get_printer_json()
            destilled_state = self.destil_printer_state(printer_json)
            aimed_lamp_state = self.calcuate_lamp_state(destilled_state)
            ## FIXME
            # lamp state
            tmp_state = self.dbus_instance.get_state()
            print("tmp_state is: ", tmp_state)
            self.dbus_instance.set_state(0)
            time.sleep(2)
            self.dbus_instance.set_state(1)
            time.sleep(2)

    def start_polling_loop(self):
        self.polling_loop()

def main():
    config_obj = ConfigExtractor()
    logging.info("Starting: Try to connect with DBus and the printer lamp interaction service...")
    dbus_bright = DBusLampBridge()
    octoprint_interface_instance = OctoprintJsonPoller(dbus_bright, config_obj.get_octopi_api_key(), config_obj.get_octopi_ip(), config_obj.get_octopi_port())
    logging.info("String up the app successful. Looking out for state changes...")
    octoprint_interface_instance.start_polling_loop()

if __name__ == "__main__":
    main()