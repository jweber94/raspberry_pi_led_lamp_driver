#include "dbus_interaction.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <cstring>

static inline const std::string DEVICE_FILE_PATH = "/home/jens/Desktop/testfile"; // For debugging
// static inline const std::string DEVICE_FILE_PATH = "/dev/printer_lamp";

namespace printer_lamp {
    DriverDbusBridge::DriverDbusBridge(std::unique_ptr<sdbus::IConnection>& connection, const bridge_config& dbus_config) : m_dbus_connection_ref{connection}, m_dbus_config{dbus_config}, m_lamp_state{-1} {
        
        using namespace std::placeholders;
        m_dbus_object = sdbus::createObject(*m_dbus_connection_ref, m_dbus_config.object_path);

        m_dbus_object->registerMethod(m_dbus_config.interface_name, "set_lamp_state", "i", "b", std::bind(&DriverDbusBridge::set_driver_state, this, _1)); // signature of the method is i => int as input parameter and b => bool as output parameter
        m_dbus_object->registerMethod(m_dbus_config.interface_name, "get_lamp_state", "i", "i", std::bind(&DriverDbusBridge::get_current_lamp_state, this, _1));
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

    void DriverDbusBridge::get_current_lamp_state(sdbus::MethodCall call) {
        // Whatever you send to it, you always get the current state. It is recommended to send the expected state
        int expected_state;
        call >> expected_state;
        if (expected_state != m_lamp_state) {
            std::cout << "WARNING: Expected state " << expected_state << " is unequal to the actual state " << m_lamp_state << "\n";
            // TODO: Maybe I will do something with this information in the future
        }
        // Make driver systemcall for the system state (read out the driver file)
        std::streampos size;
        char * memblock;
        std::ifstream file (DEVICE_FILE_PATH, std::ios::in|std::ios::binary|std::ios::ate);
        if (file.is_open())
        {
            size = file.tellg();
            memblock = new char [size];
            file.seekg (0, std::ios::beg);
            file.read (memblock, size);
            file.close();
        }
        // send the state

        int driver_state_int = lamp_state_to_int_state(memblock);

        try {
            auto reply = call.createReply();
            reply << driver_state_int;
            reply.send();
        } catch (const std::exception &exc) {
            std::cerr << "Could not send a reply from the get_current_lamp_state debus method\n";
            std::cerr << "message = " << exc.what() << "\n";
            delete[] memblock;
            return;
        }
        delete[] memblock;
    }

    int DriverDbusBridge::lamp_state_to_int_state(char* driver_state) {
        /*
        blue green white
        000 0
        100 1
        110 2
        111 3
        011 4
        001 5
        010 6
        101 7
        */

       std::cout << "DEBUG: driver_state[0] is " << driver_state << "\n";
        // if ((driver_state[0] == false) && (driver_state[1] == false) && (driver_state[2] == false)) {
        //     return 0;
        // } else if ((driver_state[0] == true) && (driver_state[1] == false) && (driver_state[2] == false)) {
        //     return 1;
        // }  else if ((driver_state[0] == true) && (driver_state[1] == true) && (driver_state[2] == false)) {
        //     return 2;
        // } else if ((driver_state[0] == true) && (driver_state[1] == true) && (driver_state[2] == true)) {
        //     return 3;
        // }  else if ((driver_state[0] == false) && (driver_state[1] == true) && (driver_state[2] == true)) {
        //     return 4;
        // }  else if ((driver_state[0] == false) && (driver_state[1] == false) && (driver_state[2] == true)) {
        //     return 5;
        // } else if ((driver_state[0] == false) && (driver_state[1] == true) && (driver_state[2] == false)) {
        //     return 6;
        // }  else if ((driver_state[0] == true) && (driver_state[1] == false) && (driver_state[2] == true)) {
        //     return 7;
        // } else {
        //     return -1;
        // }
        return -1;
    }

} /* namespace printer_lamp */