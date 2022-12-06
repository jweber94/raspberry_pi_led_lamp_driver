#include "dbus_interaction.hpp"

#include <iostream>

namespace printer_lamp {
    DriverDbusBridge::DriverDbusBridge(std::unique_ptr<sdbus::IConnection>& connection, std::string & interface_name, std::string & object_path) : m_dbus_connection_ref{connection} {
        std::cout << "Hello World!\n";
    }

    bool DriverDbusBridge::set_driver_state(const unsigned int & state) {
        std::cout << "Hello World1\n";
    }
    
    void DriverDbusBridge::send_state_change_signal() const {
        std::cout << "Hello World2\n";
    }

} /* namespace printer_lamp */