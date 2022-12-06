#include <iostream>
#include <boost/asio.hpp>


#include "dbus_interaction.hpp"
#include "config_parser.hpp"
#include "utils.hpp" 

int main(int argc, const char * argv []) {
    printer_lamp::CommandLineParser command_line_parser(argc, argv);
    const printer_lamp::bridge_config configuration = command_line_parser.get_config();
    
    std::string serviceName = "jens.printerlamp.driver_interaction";
    std::string interface_name = "jens.printerlamp";
    std::string object_path = "/3DP/printerlamp";
    auto connection = sdbus::createSystemBusConnection(serviceName);

    printer_lamp::DriverDbusBridge dbus_driver_brige_obj(connection, interface_name, object_path);

    std::cout << "Initialization finished. Starting dbus event loop...\n";
    connection->enterEventLoop();
    return 0;
}