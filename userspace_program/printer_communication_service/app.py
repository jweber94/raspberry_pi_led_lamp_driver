#!/usr/bin/python3

import dbus
import logging
import argparse
import configparser
import time
import requests
from enum import Enum

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

    def get_heating_threshold(self):
        return self.conf_parser['PRINTERSERVICE']['heating_threshold']

class PrinterState(Enum):
    STANDBY = 0
    HEATING = 1
    PRINTING = 2

########################
### DBus interaction ###
########################

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

        # TODO: Add connect_to_signal to get notified if the new state was sucessfully set - reference: https://stackoverflow.com/questions/5109879/usb-devices-udev-and-d-bus

    def get_state(self):
        #return self.printer_lamp_interface.GetLampState()
        return self.printer_lamp_interface.GetBrightness()

    def set_state(self, state):
        #self.printer_lamp_interface.SetLampState(state)
        self.printer_lamp_interface.SetBrightness(state)

#############################
### Octoprint interaction ###
#############################

class OctoprintJsonPoller():
    def __init__(self, heating_threshold, dbus_bridge, octo_api_key, ip_addr, port=80):
        self.dbus_instance = dbus_bridge
        self.octo_api_key = octo_api_key
        self.ip_addr = ip_addr
        self.port = port
        self.heating_threshold = heating_threshold
        self.bed_temp_avg = None
        self.tool_temp_avg = None

        self.octoprint_printer_state = None
        
        logging.info("Octoprint poller initialized successfully")

    def get_printer_json(self):
        header = {'X-Api-Key': self.octo_api_key}
        url = 'http://' + self.ip_addr + PRINTER_STATE_RESOURCE
        resp = requests.get(url, headers=header).json()
        if 'error' in resp.keys():
            logging.warning("Invalid response from octoprint!")
        return resp

    def destil_printer_state(self, octoprint_json):
        # check if input is valid
        if ('error' in octoprint_json.keys()) or (octoprint_json is None):
            logging.error("Invalid json for destiling the printer state received - skipping")
            return None

        # extract relevant information
        bed_temp_actual = octoprint_json.get('temperature').get('bed').get('actual')
        bed_temp_target = octoprint_json.get('temperature').get('bed').get('target')
        
        tool_temp_actual = octoprint_json.get('temperature').get('tool0').get('actual')
        tool_temp_target = octoprint_json.get('temperature').get('tool0').get('target')

        is_printing = octoprint_json.get('state').get('flags').get('printing')

        if (bed_temp_target is None) or (bed_temp_actual is None) or (tool_temp_actual is None) or (tool_temp_target is None) or (is_printing is None):
            logging.warning("Could not get the needed printer attributes for destiling the printer state") 
            return None
        
        # concatinate the extracted information to deliver it to the next processing stage
        return {'bed_temp_act': bed_temp_actual,  'bed_temp_target': bed_temp_target, 'tool_temp_act': tool_temp_actual,  'tool_temp_target': tool_temp_target, 'global_state': is_printing}

    def apply_lamp_state_rules(self, bed_temp, bed_temp_demanded, tool_temp, tool_temp_demanded, is_printing):
        is_target_temp_set = False
        if (bed_temp_demanded != 0) and (tool_temp_demanded != 0):
            is_target_temp_set = True
        
        # TODO: (Maybe) add this to the circular buffer objects and hand over their avg delta_t values to the apply_lamp_state_rules method
        delta_t_bed = bed_temp_demanded - bed_temp
        delta_t_tool = tool_temp_demanded - tool_temp
        
        log_str = "" # avoid log spam
        tmp_state = -1
        if is_printing:
            tmp_state = PrinterState.PRINTING
            log_str = "printing"
        elif (abs(delta_t_bed) > float(self.heating_threshold)) or (abs(delta_t_tool) > float(self.heating_threshold)) and not (is_printing) and is_target_temp_set:
            tmp_state = PrinterState.HEATING
            log_str = "heating"
        else:
            tmp_state = PrinterState.STANDBY
            log_str = "standby"
        if self.octoprint_printer_state != tmp_state:
            self.octoprint_printer_state = tmp_state
            logging.info("Printer state has changed to " + log_str)
    
    def calcuate_lamp_state(self, destilled_state):
        if destilled_state is None:
            logging.warning("Invalid dict for calculating the printer state received - skipping")
            return None

        if self.bed_temp_avg is None:
            self.bed_temp_avg = destilled_state.get('bed_temp_act')
        if self.tool_temp_avg is None:
            self.tool_temp_avg = destilled_state.get('tool_temp_act')

        self.apply_lamp_state_rules(destilled_state.get('bed_temp_act'), destilled_state.get('bed_temp_target'), destilled_state.get('tool_temp_act'), destilled_state.get('tool_temp_target'), destilled_state.get('global_state'))
        
        return self.octoprint_printer_state

    def get_octoprint_state(self):
        return self.octoprint_printer_state

    def polling_loop(self):
        while True:
            # octpoprint
            printer_json = self.get_printer_json()
            destilled_state = self.destil_printer_state(printer_json)
            aimed_lamp_state = self.calcuate_lamp_state(destilled_state)

            #D-Bus
            if (int(aimed_lamp_state.value) + 1) != int(self.dbus_instance.get_state()):
                self.dbus_instance.set_state(aimed_lamp_state.value)    
                time.sleep(2)
                self.dbus_instance.set_state(0)
                time.sleep(2)
            
    def start_polling_loop(self):
        self.polling_loop()

##############################
### Entrypoint interaction ###
##############################

def main():
    config_obj = ConfigExtractor()
    logging.info("Starting: Try to connect with DBus and the printer lamp interaction service...")
    dbus_bright = DBusLampBridge()
    octoprint_interface_instance = OctoprintJsonPoller(config_obj.get_heating_threshold(), dbus_bright, config_obj.get_octopi_api_key(), config_obj.get_octopi_ip(), config_obj.get_octopi_port())
    logging.info("String up the app successful. Looking out for state changes...")
    octoprint_interface_instance.start_polling_loop()

if __name__ == "__main__":
    main()
