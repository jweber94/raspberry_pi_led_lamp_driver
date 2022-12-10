#pragma once

#include <string>
#include <array>
#include <memory>
#include <sdbus-c++/sdbus-c++.h>

#include "utils.hpp"

namespace printer_lamp {
    
    struct led_state {
        bool blue;
        bool green;
        bool white;      
    };
    
    class DriverDbusBridge {
        public:
            DriverDbusBridge(std::unique_ptr<sdbus::IConnection>& connection, const bridge_config& dbus_config);
            DriverDbusBridge() = delete;

            void set_driver_state(sdbus::MethodCall call);
            void get_current_lamp_state(sdbus::MethodCall call);
            int send_state_change_signal() const;
            bool write_to_driver(int state) const;
            int lamp_state_to_int_state(char* driver_state);

        private:
            int m_lamp_state;

            std::unique_ptr<sdbus::IConnection>& m_dbus_connection_ref;
            std::unique_ptr<sdbus::IObject> m_dbus_object;
            std::array<int, 9> m_possible_states = {0, 1, 2, 3, 4, 5, 6, 7, 8};
            int m_lamp_state_translated = -1;

            const bridge_config & m_dbus_config;

    };
} /* namespace printer_lamp */
