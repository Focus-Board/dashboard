# Focusboard Dash

Firmware for the E-ink display module

### Setup

1. Add the PlatformIO extension to vscode or your editor of choice.
2. Clone this repo and initialize submodules.```git submodule init && git submodule update```
3. Create `wifi_credentials.h` under `include/` with the following contents:

```C++
#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

const char* WIFI_SSID = "YourSSID";
const char* WIFI_PASSWORD = "YourWiFiPassphrase";

#endif
```