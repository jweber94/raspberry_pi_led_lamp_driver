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
