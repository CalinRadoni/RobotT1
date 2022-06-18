# Robot T1

## Introduction

## Partitions

## OTA Update

The firmware can be updated from a HTTP or a HTTPS server.

### OTA Update from a HTTP server, for development

For development I use the `http.server` python module to start a simple HTTP server like this:

```sh
# display your IP address
ip address show up

# allow acces to the HTTP server's port in firewall
firewall-cmd --zone=public --add-port=8000/tcp

# for python 3.7+
python3 -m http.server --directory ./firmware/
# for older versions of python 3 use `cd firmware && python3 -m http.server`
```

then send the update command to the bot:

```txt
/update http://yourIPaddress:8000/firmware.bin
```

### OTA Update from GitHub

Send the update command to the bot with the full URL. As example, to load the [RobotT1](https://github.com/CalinRadoni/RobotT1)'s published firmware use:

```txt
/update https://raw.githubusercontent.com/CalinRadoni/RobotT1/main/firmware/firmware.bin
```

### OTA Update from a generic HTTPS server

Extract server's certificate in a **C** compatible output format. Here is one way to do it:

```sh
server="raw.githubusercontent.com"
echo | openssl s_client -showcerts -connect "$server":443 2>/dev/null | \
    sed -n '/-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/H; /-----BEGIN CERTIFICATE-----/h; ${g;p};' | \
    sed 's/^/"/; $!s/$/\\n" \\/; $s/$/"/'
```

See the [Extract certificates from a HTTPS server](https://calinradoni.github.io/pages/220618-extract-server-certificates.html) post for details regarding the previous code.

After you extracted the certificate it should be added to `UpdateFromWeb.cpp` and an entry for selecting it should be added to
the `UpdateFromWeb::SetCertificateForHost` function.

## Log level

In `platformio.ini` set `CORE_DEBUG_LEVEL` to a value according to `ARDUHAL_LOG_LEVEL` definitions from [esp32-hal-log.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h):

```c
#define ARDUHAL_LOG_LEVEL_NONE       (0)
#define ARDUHAL_LOG_LEVEL_ERROR      (1)
#define ARDUHAL_LOG_LEVEL_WARN       (2)
#define ARDUHAL_LOG_LEVEL_INFO       (3)
#define ARDUHAL_LOG_LEVEL_DEBUG      (4)
#define ARDUHAL_LOG_LEVEL_VERBOSE    (5)
```

**Note**: if you want colors define `CONFIG_ARDUHAL_LOG_COLORS` as **1** in `platformio.ini`.

## License

This repository is licensed under the terms of [GNU GPLv3](http://www.gnu.org/licenses/gpl-3.0.html) license. See the `LICENSE-GPLv3.txt` file.
