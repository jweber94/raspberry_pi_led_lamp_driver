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