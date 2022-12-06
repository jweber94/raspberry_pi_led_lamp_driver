#include "dbus_interaction.hpp"

#include <iostream>

namespace printer_lamp {
    DriverDbusBridge::DriverDbusBridge(std::unique_ptr<sdbus::IConnection>& connection, const bridge_config& dbus_config) : m_dbus_connection_ref{connection}, m_dbus_config{dbus_config} {
        std::cout << "DEBUG: m_dbus_config.interface_name = " << m_dbus_config.interface_name << "; m_dbus_config.object_path = " << m_dbus_config.object_path << "\n";
    }

    bool DriverDbusBridge::set_driver_state(const unsigned int & state) {
        std::cout << "Hello World1\n";
    }
    
    void DriverDbusBridge::send_state_change_signal() const {
        std::cout << "Hello World2\n";
    }

} /* namespace printer_lamp */