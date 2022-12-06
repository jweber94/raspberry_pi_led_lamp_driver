#pragma once

#include <string>
#include <boost/program_options.hpp>

#include "utils.hpp"

namespace printer_lamp {
    using namespace boost::program_options;
    class CommandLineParser {
        public:
            CommandLineParser(int & argc, const char * argv []);
            CommandLineParser() = delete;
        
            const bridge_config get_config() const;
        private:
            variables_map m_variables_map;

    };

} /* namespace printer_lamp */