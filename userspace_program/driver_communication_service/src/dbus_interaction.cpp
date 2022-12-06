#include "dbus_interaction.hpp"

#include <iostream>
#include <chrono>
#include <thread>

namespace printer_lamp {
    DriverDbusBridge::DriverDbusBridge(std::unique_ptr<sdbus::IConnection>& connection, const bridge_config& dbus_config) : m_dbus_connection_ref{connection}, m_dbus_config{dbus_config} {
        using namespace std::placeholders;
        std::cout << "DEBUG: m_dbus_config.interface_name = " << m_dbus_config.interface_name << "; m_dbus_config.object_path = " << m_dbus_config.object_path << "\n";
        m_dbus_object = sdbus::createObject(*m_dbus_connection_ref, m_dbus_config.object_path);

        m_dbus_object->registerMethod(m_dbus_config.interface_name, "set_lamp_state", "i", "b", std::bind(&DriverDbusBridge::set_driver_state, this, _1)); // signature of the method is i => int as input parameter and b => bool as output parameter
        m_dbus_object->registerSignal(m_dbus_config.interface_name, "current_lamp_state", "i");

        m_dbus_object->finishRegistration();
    }

    void DriverDbusBridge::set_driver_state(sdbus::MethodCall call) {
        // get data from request
        int demanded_state = -1;
        call >> demanded_state;
        // Return error if there are no numbers in the collection
        if (demanded_state == -1) {
            throw sdbus::Error(m_dbus_config.interface_name + ".error", "No state provided");
        }
            
        std::cout << "DEBUG: Demanded state is: " << demanded_state << "\n";

        // create answer
        auto reply = call.createReply();
        reply << demanded_state;
        reply.send();

        // setting state via driver and set signal
        std::size_t retry_counter = 0;
        while (retry_counter < 5) {
            if (this->write_to_driver(demanded_state)) {
                m_lamp_state = demanded_state;
                this->send_state_change_signal();
                break;
            } else {
                std::cout << "Could not write to driver properly. Retrying...\n";
                retry_counter++;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        if (retry_counter == 5) {
            std::cerr << "Could not write to driver after 5 retrys - something bigger might have went wrong\n";
            exit(1);
        }
    }
    
    int DriverDbusBridge::send_state_change_signal() const {
        std::cout << "Sending the signal voa dbus\n";
        // TODO
    }

    bool DriverDbusBridge::write_to_driver(int state) const {
        std::cout << "Setting driver to state " << state << "\n";
        // TODO: interact with driver file
        return true;
        return false;
    }

} /* namespace printer_lamp */