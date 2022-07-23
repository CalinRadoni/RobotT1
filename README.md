# Robot T1

## Introduction

## Partitions

The current partition scheme allows for (using a 4MB ESP32):

- two 1600 KB application partitions with OTA upgrade support
- 832 KB for a file system
- a `nvs_keys` partitions if/when [NVS Encryption](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html#nvs-encryption) will be implemented

## Camera images and power save

To save power, I am powering off the camera between pictures.

After power on the camera sensor needs time to compute `AWB`, `AGC`, `AEC` and other parameters for the current image.

First images after power on are green and dark then quality of images improves.

The time to compute those parameters depends of the current lighting conditions, image resolution and other parameters so using a fixed amount of time to wait is not a good practice.

To improve the image quality I have implemented the following algorithm:

1. power on the camera;
2. acquire an image and read `ACG` and `AEC`;
3. repeat the previous step until the `AGC` and `AEC` values are unchanged for a few consecutive readings. As a *safety* measure, the number
of repeats is limited;
4. power off the camera.

With this algorithm the first *good* image is a little purple but the next ones are OK. This behavior may be related to sensor's temperature ?

**Note:** the implementation works for **ov2640** sensor because I have added the functions to read the `AEC` and `AGC` manually in `ESPCamera.cpp`. The read functions should have been added to the `esp32-camera` library.
## I2C

The first I2C controller is mapped to the GPIO15 as SCL and GPIO14 as SDA.

The camera uses the second I2C controller.

## OTA Update

The firmware can be updated from a HTTP or a HTTPS server.

### Security note

**Warning:** In the current version the content of `credentials.h` is added unencrypted in firmware.
If you publish the firmware.bin file in a public location the credentials can be viewed easily by anyone who downloads the file.

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
