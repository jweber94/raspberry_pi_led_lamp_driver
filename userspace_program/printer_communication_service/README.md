# Octoprint interaction service

## Installation
+ You need to generate an API-Key within octoprint.
    - Go to _Settings_ --> _Application Keys_ --> _Application Identifier_ (put something in there) --> _Generate_
    - See https://docs.octoprint.org/en/master/bundledplugins/appkeys.html for details
+ TODO: Make a configuration file under `/etc/printer_lamp/lamp_configuration` where the key needs to be placed

## State deduction
The state deduction from the GET request is done by the comparison of the temperature keys from the _actual_ to the _target_ value.

+ curl request example: `$ curl -s -H "X-Api-Key: TopSecret" -X GET 192.168.2.111/api/printer`
    - Example answer from the octoprint api:
```
{
  "sd": {
    "ready": true
  },
  "state": {
    "error": "",
    "flags": {
      "cancelling": false,
      "closedOrError": false,
      "error": false,
      "finishing": false,
      "operational": true,
      "paused": false,
      "pausing": false,
      "printing": false,
      "ready": true,
      "resuming": false,
      "sdReady": true
    },
    "text": "Operational"
  },
  "temperature": {
    "bed": {
      "actual": 22.48,
      "offset": 0,
      "target": 70
    },
    "tool0": {
      "actual": 14.56,
      "offset": 0,
      "target": 0
    }
  }
}

```