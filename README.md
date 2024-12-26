# Ad blocker for ESP

Esphole is a proof-of-concept of an ad blocking DNS server for the esp32. 

## How to install

### Preprocess the hosts file
The first versions uploaded the full hosts file to the SPIFFS. The performance of finding the domain in the full hosts file was really poor, so for the current version the hosts file is preprocessed and stored in smaller files based on the lenght of the domain

    $ cd utils
    $ gen_block_lists.sh

This downloads the hosts file and saves it in different files in the *data* directory

### Upload the SPIFFS

Follow the guide of [Arduino ESP32 filesystem uploader](https://github.com/me-no-dev/arduino-esp32fs-plugin) to install the SPIFFS uploader and then select: `Tools > ESP32 Sketch Data Upload` in order to upload *data* directory to the ESP

### Upload the sketch

Open the sketch directory with Arduino editor and edit esphole.ino in order to change the WIFI settings

    #define wifi_ssid "........"
    #define wifi_password "........"

Then, upload it to the ESP

## Usage/Test

Open the serial monitor or minicom in order to find the IP of the ESP device (or scan your network in order to find it) and use nslookup in order to test some domains:


    $ nslookup - 192.168.0.161
    > github.com
    Server:		192.168.0.161
    Address:	192.168.0.161#53

    Non-authoritative answer:
    Name:	github.com
    Address: 192.30.253.112

    > analytics.google.com
    Server:		192.168.0.161
    Address:	192.168.0.161#53

    Non-authoritative answer:
    Name:	analytics.google.com
    Address: 0.0.0.0

In the serial monitor the debug information is shown:

    WiFi connected | IP address: 192.168.0.161
    DNS Server ready

    Domain: github.com | IP:192.30.253.112
    Resolv took 70 ms | Find took 57 ms

    Domain: analytics.google.com Blocked | Find took 30 ms
