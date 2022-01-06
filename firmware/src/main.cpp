/*
 * remote-relay2.ino generated by FirmataBuilder
 * Wed Jan 05 2022 13:51:45 GMT-0500 (EST)
 */

#include <ConfigurableFirmata.h>

// uncomment to enable debugging over Serial (9600 baud)
#define SERIAL_DEBUG
#include "utility/firmataDebug.h"

#include <ESP8266WiFi.h>
#include "utility/WiFiClientStream.h"
#include "utility/WiFiServerStream.h"

#define WIFI_MAX_CONN_ATTEMPTS 20

#if defined(ESP8266) && defined(SERIAL_DEBUG)
#define IS_IGNORE_PIN(p)  ((p) == 1)
#endif

#define NETWORK_PORT 3030

char ssid[] = "<YOUR_WIFI_NAME>";
char wpa_passphrase[] = "<YOUR_WIFI_PASSWORD>";

WiFiServerStream stream(NETWORK_PORT);

int connectionAttempts = 0;
bool streamConnected = false;

#include <DigitalInputFirmata.h>
DigitalInputFirmata digitalInput;

#include <DigitalOutputFirmata.h>
DigitalOutputFirmata digitalOutput;

#include <AnalogInputFirmata.h>
AnalogInputFirmata analogInput;

#include <AnalogOutputFirmata.h>
AnalogOutputFirmata analogOutput;

#include <Wire.h>
#include <I2CFirmata.h>
I2CFirmata i2c;

#include <SerialFirmata.h>
SerialFirmata serial;

#include <FirmataScheduler.h>
FirmataScheduler scheduler;

#include <FirmataExt.h>
FirmataExt firmataExt;

#include <AnalogWrite.h>

#include <FirmataReporting.h>
FirmataReporting reporting;

#include <ESPAsyncUDP.h>
AsyncUDP udp;

#include <TaskScheduler.h>
#define DEVICE_ID "IOTBRDCSTRELAY"
Scheduler taskScheduler;

void broadcastDetector();
Task broadcastTask(2000, 60, &broadcastDetector);

void systemResetCallback()
{
  for (byte i = 0; i < TOTAL_PINS; i++) {
    if (IS_PIN_ANALOG(i)) {
      Firmata.setPinMode(i, ANALOG);
    } else if (IS_PIN_DIGITAL(i)) {
      Firmata.setPinMode(i, OUTPUT);
    }
  }
  firmataExt.reset();
}

void hostConnectionCallback(byte state)
{
  switch (state) {
    case HOST_CONNECTION_CONNECTED:
      DEBUG_PRINTLN("TCP connection established");
      break;
    case HOST_CONNECTION_DISCONNECTED:
      DEBUG_PRINTLN("TCP connection disconnected");
      break;
  }
}

void printWiFiStatus()
{
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT("WiFi connection failed. Status value: ");
    DEBUG_PRINTLN(WiFi.status());
  } else {
    DEBUG_PRINTLN("Board configured as a TCP server");

    DEBUG_PRINT("SSID: ");
    DEBUG_PRINTLN(WiFi.SSID());

    DEBUG_PRINT("Local IP Address: ");
    IPAddress ip = WiFi.localIP();
    DEBUG_PRINTLN(ip);

    DEBUG_PRINT("Signal strength (RSSI): ");
    long rssi = WiFi.RSSI();
    DEBUG_PRINT(rssi);
    DEBUG_PRINTLN(" dBm");
  }
}

void ignorePins()
{
#ifdef IS_IGNORE_PIN
  // ignore pins used for WiFi controller or Firmata will overwrite their modes
  for (byte i = 0; i < TOTAL_PINS; i++) {
    if (IS_IGNORE_PIN(i)) {
      Firmata.setPinMode(i, PIN_MODE_IGNORE);
    }
  }
#endif
}

void initTransport()
{
  // IMPORTANT: if SERIAL_DEBUG is enabled, program execution will stop
  // at DEBUG_BEGIN until a Serial conneciton is established
  DEBUG_BEGIN(9600);
  DEBUG_PRINTLN("Attempting a WiFi connection using the ESP8266 WiFi library.");

  DEBUG_PRINTLN("IP will be requested from DHCP ...");

  stream.attach(hostConnectionCallback);

  DEBUG_PRINT("Attempting to connect to WPA SSID: ");
  DEBUG_PRINTLN(ssid);
  stream.begin(ssid, wpa_passphrase);

  DEBUG_PRINTLN("WiFi setup done.");

  while(WiFi.status() != WL_CONNECTED && ++connectionAttempts <= WIFI_MAX_CONN_ATTEMPTS) {
    delay(500);
    DEBUG_PRINT(".");
  }

  printWiFiStatus();

  ignorePins();

  Firmata.begin(stream);
}

void initFirmata()
{
  Firmata.setFirmwareVersion(FIRMATA_FIRMWARE_MAJOR_VERSION, FIRMATA_FIRMWARE_MINOR_VERSION);

  firmataExt.addFeature(digitalInput);
  firmataExt.addFeature(digitalOutput);
  firmataExt.addFeature(analogInput);
  firmataExt.addFeature(analogOutput);
  firmataExt.addFeature(i2c);
  firmataExt.addFeature(serial);
  firmataExt.addFeature(scheduler);
  firmataExt.addFeature(reporting);

  Firmata.attach(SYSTEM_RESET, systemResetCallback);
}

void broadcastDetector() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  udp.broadcastTo(DEVICE_ID, 3090);
}

void setup()
{
  initFirmata();

  initTransport();

  Firmata.parse(SYSTEM_RESET);

  taskScheduler.init();
  taskScheduler.addTask(broadcastTask);
  broadcastTask.enable();
}

void loop()
{
  digitalInput.report();

  while(Firmata.available()) {
    Firmata.processInput();
    if (!Firmata.isParsingMessage()) {
      goto runtasks;
    }
  }
  if (!Firmata.isParsingMessage()) {
runtasks: scheduler.runTasks();
  }

  if (reporting.elapsed()) {
    analogInput.report();
    i2c.report();
  }

  serial.update();

  stream.maintain();

  taskScheduler.execute();
}
