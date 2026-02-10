# Message Jar

[Message Jar](https://github.com/gavhu10/MessageJar/) is a messaging platform written in Python with Flask. This is a client for it, based off of the excellent [MicroCOM](https://github.com/geo-tp/MicroCOM) project by geo-tp. Also used in this project is the SdService code from the [Cardputer Game Station Emulators](https://github.com/geo-tp/Cardputer-Game-Station-Emulators/tree/xip_load), which is also made by geo-tp.

## Config

Message jar loads its configuration from `mjconfig.json` on the sd card. Here is an example configuration:  

```json
{
  "ssid": "MyWifiNetwork",
  "wifipassword": "wifi password",
  "token": "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
}
```

To get a token, create an account at [messagejar.pythonanywhere.com](https://messagejar.pythonanywhere.com) and click on your username. Then, if you navigate "Your api tokens", you should be able to create (and revoke) api tokens.

## Keybinds:

- <b>Config</b>: Use `Arrows` and `OK` button to select.
- <b>Terminal</b>: Use `Keys` and `OK` button to send messages.
