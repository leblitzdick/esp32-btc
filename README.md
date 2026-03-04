# ESP32-BTC
Simple Demo for an ESP32 module with a TFT LCD or an OLED display.

Displays current time and BTC price in USD.

Supports two different modules directly:

- ESP32 with I2C connected SSD1306 driven OLED display (resolution 128x64) made by Lolin as [WEMOS LOLIN32]( https://github.com/FablabTorino/AUG-Torino/wiki/Wemos-Lolin-board-(ESP32-with-128x64-SSD1306-I2C-OLED-display))
- ESP32 with SPI connected ST7789V driven TFT LCD display (resolution 240x135) made by TTGO as [T-Display](https://github.com/Xinyuan-LilyGO/TTGO-T-Display)

It's very easy to modify the source code for any I2C or SPI connected display.

## Arduino IDE 2.3.8 compatibility
This sketch was updated to work with the current ESP32 toolchain used by Arduino IDE **2.3.8**.

Main changes:
- replaced Tasker-based scheduling with a plain `millis()` loop
- switched to `HTTPClient` + `WiFiClientSecure` for HTTPS requests
- added robust API fallback list (no API key required)

## Free BTC APIs integrated
The sketch now tries these public/free APIs in sequence until one succeeds:
1. CoinGecko: `https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd`
2. Kraken: `https://api.kraken.com/0/public/Ticker?pair=XBTUSD`
3. Bitstamp: `https://www.bitstamp.net/api/v2/ticker/btcusd/`

The currently used API source is shown on the display.

## Required libraries
Install ESP32 core and these libraries through Arduino Library Manager:
- NTPClient
- Adafruit SSD1306 (only if `HAS_OLED` is `true`)
- Adafruit GFX (only if `HAS_OLED` is `true`)
- TFT_eSPI (only if `HAS_OLED` is `false`)

To install ESP32 core on Arduino follow the instructions on this page:
https://github.com/espressif/arduino-esp32#installation-instructions

Please note that the TTGO T-Display requires version **1.4.16** (or higher) of the TFT_eSPI library, otherwise the screen content is shifted by 52 pixels.

Enjoy!

Petr Stehlik
https://github.com/joysfera

#### LOLIN32 photo
![LOLIN photo](lolin.jpg)

#### LOLIN32 pinout
![LOLIN pinout](lolin2.jpg)

#### TTGO T-Display
![TTGO T-display photo+pinout](T-display.jpg)
