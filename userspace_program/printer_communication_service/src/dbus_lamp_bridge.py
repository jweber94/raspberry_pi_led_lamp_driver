import logging
import dbus
import time

class DBusLampBridge:
    def __init__(self):
        # DBus setup
        self.system_bus = dbus.SystemBus()
        self.reconnect_counter = 0
        self.connected_to_service = False
        while (self.reconnect_counter < 5) or (not self.connected_to_service):
            self.reconnect_counter += 1
            try:
                self.printer_lamp_proxy_dbus_test = self.system_bus.get_object('jens.printerlamp.driver_interaction', '/3DP/printerlamp')
                self.printer_lamp_interface = dbus.Interface(self.printer_lamp_proxy_dbus_test, 'jens.printerlamp')
                self.printer_lamp_interface.connect_to_signal('current_lamp_state', self.state_change_signal_cb)

                self.connected_to_service = True
            
                
            
            except:
                logging.warning("Could not connect to the DBus jens.printerlamp service. Try again")
                time.sleep(5) # sleep for 5 seconds and try again
        if not self.connected_to_service:
            logging.error("Could not connect to the jens.printerlamp dbus service after retrying. The service might not be installed! Check your system configuration and try again.")
            exit()

        # TODO: Add connect_to_signal to get notified if the new state was sucessfully set - reference: https://stackoverflow.com/questions/5109879/usb-devices-udev-and-d-bus

    def state_change_signal_cb(self, state):
        logging.info("State change invocation signal " + str(state) + " received")

    def get_state(self):
        return self.printer_lamp_interface.get_lamp_state()

    def set_state(self, state):
        self.printer_lamp_interface.set_lamp_state(state)
