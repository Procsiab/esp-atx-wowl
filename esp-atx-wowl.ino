/*
  --------------------------------
  ESP ATX Wake on Wireless LAN
  --------------------------------

  Remote control for ATX motherboard power operation and monitor
  
  Credits to https://github.com/ayushsharma82 for his work on AsyncElegantOTA, ESP-DASH and ESPConnect
  Credits to https://github.com/me-no-dev for his work on ESPAsyncTCP and ESPAsyncWebSrv
  Credits to Zvonko Bockaj for his PoC https://www.hackster.io/zvonko-bockaj/wemos-esp8266-remote-pc-switch-062c7a
  Tested with WeMos D1 Mini v2.1.0 ESP8266
*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ESPConnect.h>
#include <ElegantOTA.h>
#include <ESPDash.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#define FW_VERSION_NUM "1.2.0"
#define WIFI_HOSTNAME "esp-atx-wowl"
#define WEB_USER "management"
#define OTA_PASSWD "suchsecret"
#define DASH_PASSWD "verysecure"

#define PWR_SW_PIN D5
#define RST_PROBE_PIN D1

AsyncWebServer server(80);
ESPDash dashboard(&server);

Card power_status(&dashboard, STATUS_CARD, "Power status", "unavailable");
Card btn_quick(&dashboard, BUTTON_CARD, "Quick press power");
Card btn_long(&dashboard, BUTTON_CARD, "Long press power");
Statistic last_power_action(&dashboard, "Last power action", "unavailable");
Statistic last_power_change(&dashboard, "Last power state change", "unavailable");
Statistic firmware_version(&dashboard, "Firmware Version", FW_VERSION_NUM);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

bool is_btn_quick_pressed = false;
bool is_btn_long_pressed = false;
bool is_system_powered = false;
long switch_action_start_millis = 0;
long rst_read_millis = 0;
static long quick_press_millis = 600;
static long long_press_millis = 6000;
static long read_interval_millis = 5000;

void update_last_action_timestamp() {
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  String update_timestamp = String(ptm->tm_year+1900) + "-" + String(ptm->tm_mon+1) + "-" + String(ptm->tm_mday) + " " + timeClient.getFormattedTime();
  last_power_action.set("Last power action", update_timestamp.c_str());
}

void update_power_state_timestamp() {
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  String update_timestamp = String(ptm->tm_year+1900) + "-" + String(ptm->tm_mon+1) + "-" + String(ptm->tm_mday) + " " + timeClient.getFormattedTime();
  last_power_change.set("Last power state change", update_timestamp.c_str());
}

void close_power_switch() {
  digitalWrite(PWR_SW_PIN, HIGH);
  update_last_action_timestamp();
}

void open_power_switch() {
  digitalWrite(PWR_SW_PIN, LOW);
  update_last_action_timestamp();
}

void release_quick_power_button() {
  open_power_switch();
  btn_quick.update(false);
  dashboard.sendUpdates();
  switch_action_start_millis = 0;
  is_btn_quick_pressed = false;
}

void release_long_power_button() {
  open_power_switch();
  btn_long.update(false);
  dashboard.sendUpdates();
  switch_action_start_millis = 0;
  is_btn_long_pressed = false;
}

void hold_power_button() {
  switch_action_start_millis = millis();
  close_power_switch();
}

void update_power_status() {
  rst_read_millis = millis();
  if (digitalRead(RST_PROBE_PIN) == HIGH) {
    power_status.update("OFF", "idle");
    if (is_system_powered) {
      is_system_powered = false;
      update_power_state_timestamp();
      dashboard.sendUpdates();
    }
  } else {
    power_status.update("ON", "success");
    if (!is_system_powered) {
      is_system_powered = true;
      update_power_state_timestamp();
      dashboard.sendUpdates();
    }
  }
}

void setup(){
  Serial.begin(115200);
  Serial.println();

  // Set a custom hostname
  WiFi.hostname(WIFI_HOSTNAME);  
  // Set SSID for the configuration AP
  ESPConnect.autoConnect(WIFI_HOSTNAME);

  // Connect to previous WiFi AP or start autoConnect AP if unable to
  if(ESPConnect.begin(&server)){
    Serial.println("[ESPConnect] WiFi connected with IP: "+WiFi.localIP().toString());
  }else{
    Serial.println("[ESPConnect] Failed to connect to WiFi: rebooting...");
    delay(1000);
    ESP.restart();
  }

  // Callback on dashboard button press
  btn_quick.attachCallback([&](bool value) {
    if (!is_btn_long_pressed) {
      is_btn_quick_pressed = value;
      if (is_btn_quick_pressed) {
        hold_power_button();
      } else {
        release_quick_power_button();
      }
      Serial.println("[ESP-DASH:btn_quick] callback trigger with value: "+String((value == 1)?"true":"false"));
      btn_quick.update(value);
      dashboard.sendUpdates();
    }
  });

  btn_long.attachCallback([&](bool value) {
    if (!is_btn_quick_pressed) {
      is_btn_long_pressed = value;
      if (is_btn_long_pressed) {
        hold_power_button();
      } else {
        release_long_power_button();
      }
      Serial.println("[ESP-DASH:btn_long] callback trigger with value: "+String((value == 1)?"true":"false"));
      btn_long.update(value);
      dashboard.sendUpdates();
    }
  });

  // Set Dashboard authentication
  dashboard.setAuthentication(WEB_USER, DASH_PASSWD);
  // Inject the OTA web page endpoint
  ElegantOTA.setAuth(WEB_USER, OTA_PASSWD);
  ElegantOTA.begin(&server);
  // Run the async web server
  server.begin();

  // Start the NTP client
  timeClient.begin();
  timeClient.setTimeOffset(3600);
  timeClient.update();

  // Manage the power button and probe pins
  pinMode(PWR_SW_PIN, OUTPUT);
  pinMode(RST_PROBE_PIN, INPUT);
  open_power_switch();
  update_power_status();
}

void loop(){
  // Handle reboot after firmware upload
  ElegantOTA.loop();
  // Handle button press and power events
  if (is_btn_quick_pressed && switch_action_start_millis != 0 && millis() - switch_action_start_millis > quick_press_millis) {
    release_quick_power_button();
    Serial.println("[btn_quick] automatic release after quick delay");
  }
  if (is_btn_long_pressed && switch_action_start_millis != 0 && millis() - switch_action_start_millis > long_press_millis) {
    release_long_power_button();
    Serial.println("[btn_long] automatic release after long delay");
  }
  if (millis() - rst_read_millis > read_interval_millis) {
    update_power_status();
  }
}
