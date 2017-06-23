
C code to read DHT11/22 temperature and humidity values and send them to MQTT server
What it does is:
- scan WIFI for known access points
- if AP found, connect wifi, set time using NTP server, connect to MQTT server using TLS and start poling DHT sensor and publish temperature and humidity. 
- publish also free ram and uptime, for test purpose
- blinking LED as in the NodeMcu project.

NOTE: crash when server require certificate. Using ESP8266 2.1.0 board installed from Arduino IDE.

Lavaux Gilles 2017/05
