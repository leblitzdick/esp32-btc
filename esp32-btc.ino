/********************************************
    ESP32 LCD TFT / OLED display demo

    Prints current time and BTC price in USD

    originally written by Petr Stehlik in 2019/07/25
    updated for Arduino IDE 2.3.8 in 2026

    released under the GNU GPL

    https://github.com/joysfera/
*********************************************/

#define HAS_OLED false  // TTGO T-Display by default, change to true for OLED SSD1306

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>  // https://github.com/arduino-libraries/NTPClient

#if HAS_OLED
# include <Wire.h>
# include <Adafruit_GFX.h>
# include <Adafruit_SSD1306.h>
#else
# include <TFT_eSPI.h>
# include <SPI.h>
#endif

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#if HAS_OLED
Adafruit_SSD1306 disp(128, 64, &Wire, -1);
#else
# define TFT_BL          4  // Display backlight control pin
TFT_eSPI disp = TFT_eSPI(135, 240);
#endif

const char* ssid = "yourssid";          // your network SSID (name of wifi network)
const char* password = "yourpassword";  // your network password

const unsigned long REFRESH_MS = 100000UL;
unsigned long nextRefreshAt = 0;

struct ApiEndpoint {
    const char* name;
    const char* url;
};

const ApiEndpoint APIS[] = {
    {"CoinGecko", "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd"},
    {"Kraken", "https://api.kraken.com/0/public/Ticker?pair=XBTUSD"},
    {"Bitstamp", "https://www.bitstamp.net/api/v2/ticker/btcusd/"}
};

String lastSource = "none";
int lastBTC = -1;

static int parseCoinGecko(const String& json)
{
    int keyPos = json.indexOf("\"usd\":");
    if (keyPos < 0) {
        return -1;
    }

    int valueStart = keyPos + 6;
    int valueEnd = valueStart;
    while (valueEnd < (int)json.length() && (isDigit(json[valueEnd]) || json[valueEnd] == '.')) {
        valueEnd++;
    }

    return json.substring(valueStart, valueEnd).toFloat();
}

static int parseKraken(const String& json)
{
    int keyPos = json.indexOf("\"c\":[\"");
    if (keyPos < 0) {
        return -1;
    }

    int valueStart = keyPos + 6;
    int valueEnd = json.indexOf('"', valueStart);
    if (valueEnd < 0) {
        return -1;
    }

    return json.substring(valueStart, valueEnd).toFloat();
}

static int parseBitstamp(const String& json)
{
    int keyPos = json.indexOf("\"last\":\"");
    if (keyPos < 0) {
        return -1;
    }

    int valueStart = keyPos + 8;
    int valueEnd = json.indexOf('"', valueStart);
    if (valueEnd < 0) {
        return -1;
    }

    return json.substring(valueStart, valueEnd).toFloat();
}

static int parseBTC(const String& apiName, const String& payload)
{
    if (apiName == "CoinGecko") {
        return parseCoinGecko(payload);
    }
    if (apiName == "Kraken") {
        return parseKraken(payload);
    }
    if (apiName == "Bitstamp") {
        return parseBitstamp(payload);
    }

    return -1;
}

static int fetchBTCFromApi(const ApiEndpoint& api)
{
    WiFiClientSecure secureClient;
    secureClient.setInsecure();  // keep setup simple across the APIs listed above

    HTTPClient https;
    if (!https.begin(secureClient, api.url)) {
        Serial.printf("[%s] HTTPS begin failed\n", api.name);
        return -1;
    }

    https.setConnectTimeout(8000);
    https.setTimeout(8000);

    int code = https.GET();
    if (code <= 0 || code != HTTP_CODE_OK) {
        Serial.printf("[%s] HTTP error: %d\n", api.name, code);
        https.end();
        return -1;
    }

    String payload = https.getString();
    https.end();

    int price = parseBTC(api.name, payload);
    if (price <= 0) {
        Serial.printf("[%s] Parse failed\n", api.name);
        return -1;
    }

    Serial.printf("[%s] BTC = $%d\n", api.name, price);
    return price;
}

void setup()
{
    Serial.begin(115200);
    delay(100);

#if HAS_OLED
    Wire.begin(5, 4);
    if (!disp.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
        Serial.println(F("SSD1306 init failed"));
        for (;;) ; // no display no fun
    }
    disp.setTextColor(WHITE);
#else
    if (TFT_BL > 0) {
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, HIGH);
    }
    disp.init();
    disp.setRotation(1);
    disp.fillScreen(TFT_BLACK);
    disp.setTextColor(TFT_WHITE, TFT_BLACK);
    disp.setTextDatum(TL_DATUM);
#endif

    disp.print("Connecting to WiFi");
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        disp.print('.');
        Serial.print('.');
        delay(1000);
    }
    Serial.println(" OK");

    timeClient.begin();
    timeClient.setTimeOffset(3600 + 3600); // CEST
    timeClient.update();

    pinMode(0, INPUT_PULLUP);
    displayBTC();
}

void loop()
{
    timeClient.update();

    if (digitalRead(0) == LOW) {
        delay(50);
        if (digitalRead(0) == LOW) {
            displayBTC();
            while (digitalRead(0) == LOW) {
                delay(20);
            }
        }
    }

    if (millis() >= nextRefreshAt) {
        displayBTC();
    }

    delay(50);
}

int getBTC(void)
{
    for (size_t i = 0; i < sizeof(APIS) / sizeof(APIS[0]); ++i) {
        int price = fetchBTCFromApi(APIS[i]);
        if (price > 0) {
            lastSource = APIS[i].name;
            return price;
        }
    }

    return -1;
}

void displayBTC(void)
{
    lastBTC = getBTC();

#if HAS_OLED
    disp.clearDisplay();
#else
    disp.fillScreen(TFT_BLACK);
#endif

    disp.setTextSize(HAS_OLED ? 2 : 3);
    disp.setCursor(0, 0);
    disp.print(timeClient.getFormattedTime());

    disp.setTextSize(HAS_OLED ? 1 : 2);
    disp.setCursor(0, HAS_OLED ? 16 : 28);
    disp.print("API: ");
    disp.print(lastSource);

    disp.setTextSize(HAS_OLED ? 2 : 3);
    disp.setCursor(0, HAS_OLED ? 26 : 50);
    disp.print("BTC USD:");

    disp.setTextSize(HAS_OLED ? 3 : 5);
    disp.setCursor(0, HAS_OLED ? 42 : 78);
    if (lastBTC > 0) {
        disp.print('$');
        disp.print(lastBTC);
    } else {
        disp.print("n/a");
    }

#if HAS_OLED
    disp.display();
#endif

    nextRefreshAt = millis() + REFRESH_MS;
}
