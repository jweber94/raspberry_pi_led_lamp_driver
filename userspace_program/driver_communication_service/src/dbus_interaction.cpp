#include "dbus_interaction.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <filesystem>

static inline const std::string DEVICE_FILE_PATH = "/home/jens/Desktop/testfile"; // For debugging
// static inline const std::string DEVICE_FILE_PATH = "/dev/printer_lamp";

namespace printer_lamp {
    DriverDbusBridge::DriverDbusBridge(std::unique_ptr<sdbus::IConnection>& connection, const bridge_config& dbus_config) : m_dbus_connection_ref{connection}, m_dbus_config{dbus_config} {
        
        using namespace std::placeholders;
        m_dbus_object = sdbus::createObject(*m_dbus_connection_ref, m_dbus_config.object_path);

        m_dbus_object->registerMethod(m_dbus_config.interface_name, "set_lamp_state", "i", "b", std::bind(&DriverDbusBridge::set_driver_state, this, _1)); // signature of the method is i => int as input parameter and b => bool as output parameter
        m_dbus_object->registerSignal(m_dbus_config.interface_name, "current_lamp_state", "i");

        m_dbus_object->finishRegistration();
    }

    void DriverDbusBridge::set_driver_state(sdbus::MethodCall call) {
        // get data from request
        int demanded_state = -1;
        call >> demanded_state;
        
        // answer the request
        try {
            auto reply = call.createReply();
            if ((demanded_state == -1) || (std::find(m_possible_states.begin(), m_possible_states.end(), demanded_state) == std::end(m_possible_states))) {
                std::cout << "Invalid request detected. Sending error reply\n";
                reply << false;
                reply.send();
                return;
            }
            reply << true;
            reply.send();
        } catch (...) {
            std::cerr << "Could not send reply\n";
            exit(1);
        }
            
        // setting state via driver and set signal
        std::size_t retry_counter = 0;
        while (retry_counter < 5) {
            if (this->write_to_driver(demanded_state)) {
                std::cout << "State change to " << demanded_state << " successful\n";
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
        std::cout << "Sending the signal via dbus\n";
        auto signal = m_dbus_object.get()->createSignal(m_dbus_config.interface_name, "current_lamp_state");
        signal << m_lamp_state;
        m_dbus_object.get()->emitSignal(signal);
    }

    bool DriverDbusBridge::write_to_driver(int state) const {
        std::cout << "Setting driver to state " << state << "\n";
        if (!std::filesystem::exists(DEVICE_FILE_PATH)) {
            std::cout << "Driver file does not exist\n";
            return false;
        }

        std::ofstream driver_file (DEVICE_FILE_PATH);
        if (driver_file.is_open())
        {
          driver_file << state;
          driver_file.close();
          return true;
        }
        return false;
    }

} /* namespace printer_lamp */