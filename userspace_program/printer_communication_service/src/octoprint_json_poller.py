import time
import requests
from enum import Enum

from src.moving_avg_buffer import *

# Octopi url
PRINTER_STATE_RESOURCE = "/api/printer"

class PrinterState(Enum):
    STANDBY = 0
    HEATING = 1
    PRINTING = 2

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
        self.last_destilled_state = None
        self.state_changed = None

        self.bed_target_val = None
        self.bed_actual_moving_avg_obj = MovingAvgRingbuffer(3)
        self.tool_target_val = None
        self.tool_actual_moving_avg_obj = MovingAvgRingbuffer(3)

        logging.info("Octoprint poller initialized successfully")

    def get_printer_json(self):
        header = {'X-Api-Key': self.octo_api_key}
        url = 'http://' + self.ip_addr + PRINTER_STATE_RESOURCE
        resp = requests.get(url, headers=header).json()
        return resp

    def destil_printer_state(self, octoprint_json):
        # check if input is valid
        if ('error' in octoprint_json.keys()) or (octoprint_json is None):
            logging.error("Invalid json for destiling the printer state received - skipping")
            return None

        # extract relevant information
        bed_temp_actual = octoprint_json.get('temperature').get('bed').get('actual')
        tool_temp_actual = octoprint_json.get('temperature').get('tool0').get('actual')

        is_printing = octoprint_json.get('state').get('flags').get('printing')

        # calculate moving average to avoid state jitter
        self.tool_target_val = octoprint_json.get('temperature').get('tool0').get('target')
        self.bed_target_val = octoprint_json.get('temperature').get('bed').get('target')
        self.tool_actual_moving_avg_obj.enqueue(tool_temp_actual)
        self.bed_actual_moving_avg_obj.enqueue(bed_temp_actual)

        if (self.bed_target_val is None) or (bed_temp_actual is None) or (tool_temp_actual is None) or (self.tool_target_val is None) or (is_printing is None):
            logging.warning("Could not get the needed printer attributes for destiling the printer state") 
            return None

        # concatinate the extracted information to deliver it to the next processing stage
        return {'bed_temp_act': self.bed_actual_moving_avg_obj.get_moving_avg(),  'bed_temp_target': self.bed_target_val, 'tool_temp_act': self.tool_actual_moving_avg_obj.get_moving_avg(),  'tool_temp_target': self.tool_target_val, 'global_state': is_printing}

    def apply_lamp_state_rules(self, bed_temp, bed_temp_demanded, tool_temp, tool_temp_demanded, is_printing):
        is_target_temp_set = False
        if (bed_temp_demanded != 0.0) or (tool_temp_demanded != 0.0):
            is_target_temp_set = True
        
        # TODO: (Maybe) add this to the circular buffer objects and hand over their avg delta_t values to the apply_lamp_state_rules method
        delta_t_bed = bed_temp_demanded - bed_temp
        delta_t_tool = tool_temp_demanded - tool_temp
        logging.debug("delta_t_tool = " + str(delta_t_tool))

        log_str = "" # avoid log spam
        tmp_state = -1
        delta_t_bed_exists = abs(delta_t_bed) > float(self.heating_threshold)
        delta_t_tool_exists = abs(delta_t_tool) > float(self.heating_threshold)

        is_delta_t_existing = delta_t_bed_exists or delta_t_tool_exists

        if is_printing and (not is_delta_t_existing):
            tmp_state = PrinterState.PRINTING
            log_str = "printing"
        elif is_delta_t_existing and is_target_temp_set:
            tmp_state = PrinterState.HEATING
            log_str = "heating"
        else:
            tmp_state = PrinterState.STANDBY
            log_str = "standby"

        if self.octoprint_printer_state != tmp_state:
            self.octoprint_printer_state = tmp_state
            self.state_changed = True
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
        
    def get_octoprint_state(self):
        return self.octoprint_printer_state

    def polling_loop(self):
        while True:
            # octpoprint
            printer_json = self.get_printer_json()
            if "error" in printer_json.keys():
                logging.warning("Invalid response from octoprint!")
                self.dbus_instance.set_state(8) # turn all lights off in case of invalid json from octoprint
            
            destilled_state = self.destil_printer_state(printer_json)
            #aimed_lamp_state = self.calcuate_lamp_state(destilled_state)
            self.calcuate_lamp_state(destilled_state)

            #D-Bus
            # driver_state = int(self.dbus_instance.get_lamp_state(aimed_lamp_state.value)) # maybe we will use this later
            if self.state_changed:
                logging.info("Update dbus to state " + str(destilled_state))
                self.state_changed = False
                self.send_polling_state_to_driver(self.octoprint_printer_state)
                self.last_destilled_state = destilled_state
            
            time.sleep(5)
            
    def start_polling_loop(self):
        self.polling_loop()

    def send_polling_state_to_driver(self, aimed_state):
        self.dbus_instance.set_state(6) # lightplay 1
        if aimed_state == PrinterState.STANDBY:
            logging.info("STANDBY")
            self.dbus_instance.set_state(8) # reset all lights call
            self.dbus_instance.set_state(0)
        elif aimed_state == PrinterState.HEATING:
            logging.info("HEATING")
            self.dbus_instance.set_state(8)
            self.dbus_instance.set_state(1)
        elif aimed_state == PrinterState.PRINTING:
            logging.info("PRINTING")
            self.dbus_instance.set_state(8)
            self.dbus_instance.set_state(2)
        
