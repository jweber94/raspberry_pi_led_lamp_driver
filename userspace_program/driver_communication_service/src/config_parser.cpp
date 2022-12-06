#include "config_parser.hpp"

#include <boost/program_options.hpp>
#include <iostream>

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

    const bridge_config CommandLineParser::get_config() const {
        bridge_config parsing_result;

        // TODO: Use ini parser and m_variables_map
        /*

        else if (m_variables_map.count("age"))
            std::cout << "Age: " << m_variables_map["age"].as<int>() << '\n';
        else if (m_variables_map.count("pi"))
            std::cout << "Pi: " << m_variables_map["pi"].as<float>() << '\n';
        */
        
        parsing_result.interface_name = "Foo";
        parsing_result.object_path = "Bar";

        return parsing_result;
    }
}