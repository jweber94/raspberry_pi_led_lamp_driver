#!/usr/bin/python3

import dbus
import logging
import argparse
import time
import requests

#################
### Utilities ###
#################

# command line parser
parser = argparse.ArgumentParser(description='Printer interaction program')
parser.add_argument('--log_level', type=str, required=True, help='Log level (debug, info, warning, error, critical)')
command_line_args = parser.parse_args()

def get_log_level():
    received_log_level = command_line_args.log_level
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


def init_logger():
    # Logger init
    logging.basicConfig(format='%(levelname)s:%(message)s', level=get_log_level())
    logging.info("Logger initialized")
    
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
    def __init__(self, dbus_bridge):
        self.current_octo_state = None
        self.dbus_instance = dbus_bridge
        logging.info("Octoprint poller initialized successfully")

def main():
    init_logger()
    logging.info("Starting: Try to connect with DBus and the printer lamp interaction service...")
    dbus_bright = DBusLampBridge()
    octoprint_interface_instance = OctoprintJsonPoller(dbus_bright)
    current_state = dbus_bright.get_state()
    print(current_state)

    logging.info("String up the app successful. Waiting for events...")

if __name__ == "__main__":
    main()