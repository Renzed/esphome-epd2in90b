# esphome-epd2in15b

ESPHome external component for the **Waveshare 2.15inch e-Paper HAT+ (B)** — a 160×296 tri-color (black/white/red) e-ink display.

## Why a custom component?

ESPHome's built-in `waveshare_epaper` platform does not support this display. The 2.15" B uses an SSD1680-family controller with a specific init sequence, and critically requires **two separate framebuffers** — one for black pixels and one for red pixels — where the red buffer must be bitwise-inverted before sending. Generic drivers leave the red plane in an undefined state, causing red noise on the display.

## Hardware

- **Display**: [Waveshare 2.15inch e-Paper HAT+ (B)](https://www.waveshare.com/2.15inch-e-paper-hat-plus-b.htm)
- **Tested with**: DFRobot FireBeetle 2 ESP32-C6
- **Interface**: SPI
- **Colors**: Black, White, Red
- **Resolution**: 160×296 px

## Wiring (FireBeetle 2 ESP32-C6)

| HAT+ Pin | ESP32-C6 GPIO | Notes                        |
|----------|---------------|------------------------------|
| VCC      | 3.3V          |                              |
| GND      | GND           |                              |
| DIN      | GPIO22        | MOSI (labeled MO)            |
| CLK      | GPIO23        | SCK                          |
| CS       | GPIO19        |                              |
| DC       | GPIO20        |                              |
| RST      | GPIO18        |                              |
| BUSY     | GPIO14        |                              |
| PWR      | GPIO1         | Active-high display power    |

## Usage

Reference this repo in your ESPHome YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/zAAmpie/esphome-epd2in15b
      ref: main
    components: [epd2in15b]
```

## Full example YAML

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/zAAmpie/esphome-epd2in15b
      ref: main
    components: [epd2in15b]

spi:
  clk_pin: GPIO23
  mosi_pin: GPIO22

output:
  - platform: gpio
    pin: GPIO1
    id: display_power

switch:
  - platform: output
    output: display_power
    id: display_pwr_switch
    name: "Display Power"
    restore_mode: ALWAYS_ON

font:
  - file: "gfonts://Roboto"
    id: font_large
    size: 32
  - file: "gfonts://Roboto"
    id: font_small
    size: 16

display:
  - platform: epd2in15b
    id: epaper
    cs_pin: GPIO19
    dc_pin: GPIO20
    reset_pin: GPIO18
    busy_pin: GPIO14
    update_interval: never
    lambda: |-
      it.fill(Color::WHITE);
      it.print(
        it.get_width() / 2,
        it.get_height() / 2,
        id(font_large),
        Color::BLACK,
        TextAlign::CENTER,
        "Hello!"
      );
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
- The PWR pin on the HAT+ controls power via an NPN+PMOS circuit — active-high (HIGH = display on).
