
C code to read DHT11/22 temperature and humidity values and send them to MQTT server
What it does is:
- scan WIFI for known access points
- if AP found, connect wifi, connect to MQTT server using TLD and start poling DHT sensor and publish temperature and humidity. 
- publish also free ram and uptime, for test purpose
  
TODO:
- blinking LED as in the NodeMcu project.

Lavaux Gilles 2017/05
