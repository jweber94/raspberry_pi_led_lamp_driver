#include <iostream>
#include <boost/asio.hpp>


#include "dbus_interaction.hpp"
#include "config_parser.hpp"
#include "utils.hpp" 

static inline const std::string SERVICE_NAME = "jens.printerlamp.driver_interaction";

int main(int argc, const char * argv []) {
    printer_lamp::CommandLineParser command_line_parser(argc, argv);
    const printer_lamp::bridge_config configuration = command_line_parser.get_config();
    
    if (configuration.interface_name == "UNKNOWN" || configuration.object_path == "UNKNOWN") {
        std::cout << "Invalid config file received. Program is unable to start\n";
        exit(1);
    }
    
    auto connection = sdbus::createSystemBusConnection(SERVICE_NAME);
    printer_lamp::DriverDbusBridge dbus_driver_brige_obj(connection, configuration);

    std::cout << "Initialization finished. Starting dbus event loop...\n";
    connection->enterEventLoop();
    return 0;
}