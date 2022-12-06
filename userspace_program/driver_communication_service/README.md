# Driver interaction service
This is the driver interaction service. It waits for state change requests by the octoprint interaction service and then changes the state of the printer lamp via its kernel driver. After the state of the lamp has changed, it will send a signal to dbus to notify the octoprint interaction service that the state change was successful.

## Debug configuration with VSCode
Here is the settings.json:

```
{
    "cmake.debugConfig": {
        "args":[
            
        ]
    }
}
```

## DBus library
+ Since the libdbus library says ["_You will save yourself a lot of pain if you use a higher-level wrapper or a reimplementation._"](https://www.freedesktop.org/wiki/Software/dbus/), I choose to use a more convenient to use library for the interaction with DBus, called [dbus-cpp](https://github.com/Kistler-Group/sdbus-cpp/blob/master/docs/using-sdbus-c++.md#integrating-sdbus-c-into-your-project)
    - To find a Dbus library that is applicable to your programming language, see https://www.freedesktop.org/wiki/Software/DBusBindings/

## Requirements
+ Since sdbus-cpp depends on libsystemd, we need to install the development headers on our target system for compilation: `$ sudo apt install libsystemd-dev`
+ In order to use boost and dbus-cpp properly, we need to use the following `~/.conan/profiles/default` configuration, according to https://stackoverflow.com/questions/55692966/boost-link-error-using-conan-package-manager:
```
[settings]
os=Linux
os_build=Linux
arch=x86_64
arch_build=x86_64
compiler=gcc
compiler.version=9
#compiler.libcxx=libstdc++
compiler.libcxx=libstdc++11
build_type=Release
[options]
[build_requires]
[env]
```

## Configuration
+ The configuration file is placed within `./config/driver_service.ini`. This will be located under `/etc/octolamp/driver_service.ini`.
+ Besides the service configuration, we need to define a DBus configuration file like it is mentioned within [this](https://github.com/Kistler-Group/sdbus-cpp/blob/master/docs/systemd-dbus-config.md#dbus-configuration) explaination, to let the service connect to Dbus properly. This configuration file can also be found under `./config/jens.printerlamp.driver_interaction.conf`