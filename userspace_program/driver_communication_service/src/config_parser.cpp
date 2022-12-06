#include "config_parser.hpp"

#include <boost/program_options.hpp>
#include <iostream>
#include <INIReader.h>

namespace printer_lamp {

    CommandLineParser::CommandLineParser(int & argc, const char * argv []) {
        try
        {
            options_description desc{"Options"};
          
            desc.add_options()
                ("help,h", "Help screen")
                ("config_path", value<std::string>()->default_value("/etc/octolamp/driver_service.ini"), "Path to the config file for the driver interaction service");

          
            store(parse_command_line(argc, argv, desc), m_variables_map);
            notify(m_variables_map);

            if (m_variables_map.count("help")) {
                std::cout << desc << '\n';
            }
        }
        catch (const error &ex)
        {
          std::cerr << ex.what() << '\n';
        }
    }

    bridge_config CommandLineParser::get_config() {
        if (m_bridge_config.interface_name == "" && m_bridge_config.object_path == "") {
            try {
                std::string path_to_config = m_variables_map["config_path"].as<std::string>();
                INIReader reader(path_to_config);
                m_bridge_config.interface_name =  reader.Get("DRIVERSERVICE", "interface_name", "UNKNOWN");
                m_bridge_config.object_path = reader.Get("DRIVERSERVICE", "object_path", "UNKNOWN");
            } catch (...) {
                std::cout << "Could not parse config file\n";
                exit(1);
            }
        }
        
        return m_bridge_config;
    }
}