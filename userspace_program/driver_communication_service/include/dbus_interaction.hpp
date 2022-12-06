#pragma once

#include <string>

#include <memory>
#include <sdbus-c++/sdbus-c++.h>

namespace printer_lamp {
    class DriverDbusBridge {
        public:
            DriverDbusBridge(std::unique_ptr<sdbus::IConnection>& connection, std::string & interface_name, std::string & object_path);
            DriverDbusBridge() = delete;

            bool set_driver_state(const unsigned int & state);
            void send_state_change_signal() const;

        private:
            const std::string m_object_path;
            const std::string m_interface_name;
            unsigned int m_lamp_state;

            std::unique_ptr<sdbus::IConnection>& m_dbus_connection_ref;
            sdbus::IObject* m_dbus_object;

        };
} /* namespace printer_lamp */
