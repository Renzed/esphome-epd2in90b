# esphome-epd2in15b

ESPHome external component for the **ZHONGJINGYUAN 2.9inch e-Paper display** — a 128×296 tri-color (black/white/red) e-ink display.
This is adapted from [https://github.com/zAAmpie/esphome-epd2in15b]

## Why a custom component?

ESPHome's built-in epd platforms do not support this display. The display uses an SSD1680-family controller with a specific init sequence, and critically requires **two separate framebuffers**, one for black pixels and one for red pixels. Generic drivers leave the red plane in an undefined state, causing red noise on the display.

## Hardware

- **Display**: [ZHONGJINGYUAN 2.9inch e-Paper display](https://www.tinytronics.nl/nl/displays/e-ink/2.9-inch-e-ink-e-paper-display-zwart-wit-rood)
- **Tested with**: [ESP32 dev board](https://www.tinytronics.nl/nl/development-boards/microcontroller-boards/met-wi-fi/esp32-wifi-en-bluetooth-board-cp2102)
- **Interface**: SPI
- **Colors**: Black, White, Red
- **Resolution**: 128×296 px

## Wiring

| EPD Pin  | ESP32 GPIO    |
|----------|---------------|
| VCC      | 3.3V          |
| GND      | GND           |
| CLK      | GPIO18        |
| SDA      | GPIO23        |
| CS       | GPIO33        |
| DC       | GPIO25        |
| RST      | GPIO32        |
| BUSY     | GPIO26        |

## Usage

Reference this repo in your ESPHome YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/Renzed/esphome-epd2in90b/
      ref: main
    components: [epd2in15b]
    refresh: 1min
```

## Full example YAML

```yaml
esphome:
  name: luchtkwaliteit--sen66
  friendly_name: Luchtkwaliteit - Sen66

esp32:
  board: esp32dev
  framework:
    type: esp-idf

# Enable logging
logger:

# Enable Home Assistant API
api:
  encryption:
    key: 

ota:
  - platform: esphome
    password: 

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

  # Enable fallback hotspot (captive portal) in case wifi connection fails
  ap:
    ssid: "Luchtkwaliteit--Sen66"
    password: 

external_components:
  - source:
      type: git
      url: https://github.com/Renzed/esphome-epd2in90b/
      ref: main
    components: [epd2in15b]
    refresh: 1min

captive_portal:
    
i2c:
  sda: GPIO21
  scl: GPIO22

sensor:
  - platform: sen6x
    type: SEN66
    pm_1_0:
      name: "PM 1.0"
      id: pm10
    pm_2_5:
      name: "PM 2.5"
      id: pm25
    pm_4_0:
      name: "PM 4.0"
      id: pm40
    pm_10_0:
      name: "PM 10.0"
      id: pm100
    temperature:
      name: "Temperatuur"
      id: temp
    humidity:
      name: "Luchtvochtigheid"
      id: humidity
    voc:
      name: "VOC index"
      id: voc
    nox:
      name: "NOx index"
      id: nox
    co2:
      name: "CO2"
      id: co2

spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23

font:
  - file: "gfonts://Roboto"
    id: roboto_20
    size: 20

epd2in15b:
  id: hi
  cs_pin: GPIO33
  dc_pin: GPIO25
  reset_pin: GPIO32
  busy_pin: GPIO26
  rotation: 90
  update_interval: 60s
  lambda: |-
    it.fill(Color::WHITE);
    Color color = Color::BLACK;
    char col_buf[15];
    snprintf(col_buf, 15, "r%d g%d b%d", color.r, color.g, color.b);
    ESP_LOGD("hiha", col_buf);
    it.printf(0, 0, id(roboto_20), color, "Temp: %.1f", id(temp).state); 
    it.printf(0, 22, id(roboto_20), color, "Vocht: %.0f%%", id(humidity).state);
    if(id(voc).state>120){
      color = Color(255,0,0);
    } else {
      color = Color::BLACK;
    }
    it.printf(0, 44, id(roboto_20), color, "VOC: %.0f", id(voc).state);
    if(id(nox).state>1){
      color = Color(255,0,0);
    } else {
      color = Color::BLACK;
    }
    it.printf(0, 66, id(roboto_20), color, "NOx: %.0f", id(nox).state);
    if(id(co2).state>800){
      color = Color(255,0,0);
    } else {
      color = Color::BLACK;
    }
    it.printf(0, 88, id(roboto_20), color, "CO2: %.0f", id(co2).state);
    if(id(pm10).state>5.0){
      color = Color(255,0,0);
    } else {
      color = Color::BLACK;
    }
    it.printf(148, 0, id(roboto_20), color, "PM 1.0: %.1f", id(pm10).state);
    if(id(pm25).state>5.0){
      color = Color(255,0,0);
    } else {
      color = Color::BLACK;
    }
    it.printf(148, 22, id(roboto_20), color, "PM 2.5: %.1f", id(pm25).state);
    if(id(pm40).state>10.0){
      color = Color(255,0,0);
    } else {
      color = Color::BLACK;
    }
    it.printf(148, 44, id(roboto_20), color, "PM 4.0: %.1f", id(pm40).state);
    if(id(pm100).state>15.0){
      color = Color(255,0,0);
    } else {
      color = Color::BLACK;
    }
    it.printf(148, 66, id(roboto_20), color, "PM 10.0: %.1f", id(pm100).state);
```

## Color usage in lambda

```cpp
Color::WHITE   // white pixel
Color::BLACK   // black pixel
Color(255,0,0) // red pixel
```

## Notes

- The display takes approximately 15–20 seconds for a full tri-color refresh. This is normal for red/black/white e-ink panels.
- For battery-powered use, add a `delay: 20s` after `component.update: epaper` before entering deep sleep to allow the refresh to complete.
