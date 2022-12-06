#pragma once

#include <string>

#include <memory>
#include <sdbus-c++/sdbus-c++.h>

#include "utils.hpp"

namespace printer_lamp {
    class DriverDbusBridge {
        public:
            DriverDbusBridge(std::unique_ptr<sdbus::IConnection>& connection, const bridge_config& dbus_config);
            DriverDbusBridge() = delete;

            void set_driver_state(sdbus::MethodCall call);
            int send_state_change_signal() const;
            bool write_to_driver(int state) const;

        private:
            unsigned int m_lamp_state;

            std::unique_ptr<sdbus::IConnection>& m_dbus_connection_ref;
            std::unique_ptr<sdbus::IObject> m_dbus_object;

            const bridge_config & m_dbus_config;

    };
} /* namespace printer_lamp */
