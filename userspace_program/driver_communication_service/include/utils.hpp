#pragma once

#include <string>

namespace printer_lamp {
    // configuration_object
    struct bridge_config {
        std::string object_path {""};
        std::string interface_name {""};
    };

} /* namespace printer_lamp */