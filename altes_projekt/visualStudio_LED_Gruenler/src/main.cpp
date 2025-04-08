// Name:       Peter Thieme
// Projekt:    LED_Grünler
// Controler:  ESP32C3 Super mini
// Board:      Nologo ESP32C3 Super Mini
// Info:
//-----------------------------------------//

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Wire.h"
#include "modi.h"

#define PIN 10            // LED PIN //10 ist wahrscheinlich durch
#define NUMPIXELS 80      // Anzahl Led's
#define I2C_DEV_ADDR 0x33 // Adresse des Slaves
// Mögliche Slave-Adressen: { 0x15, 0x25, 0x35, 0x45, 0x11, 0x22, 0x33 };
//                              1     2     3     4     5     6     7
//                            COM3  COM7  COM10 COM4  COM9  COM11 COM8
uint32_t i = 0; // Anzahl bits die übermittelt werden (beim Zurücksenden)
int Status = 0; // Status der übermittelt wird
int Status_wert, Old_Status, Old_Status_wert;
bool ausgeschalten = false;
byte R = 0;
byte G = 0;
byte B = 0;
byte W = 0; // Parameter LED-streifen

uint8_t Brightness = 50;
uint8_t last_Brightness;
uint8_t speed_value = 10; // stellt die geschwindigkeit der Animationen ein (10 NORMAL)

bool farbdarstellung = false; // Zeigt streifen eine farbe an?
int timer = 0;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_WRGB + NEO_KHZ800);

void onRequest()
{
    /* if (Status == 9 && Status_wert == 0)
     {
         Wire.write(11);
     }
     else if (animation_ende == true)
     {
         Wire.write(11);
         animation_ende = false;
     }
     else
         Wire.write(0);
    */
}

void onReceive(int len)
{
    // Serial.printf("onReceive[%d]: ", len);
    while (Wire.available())
    {
        // Status = Wire.read();
        byte highByte1 = Wire.read();
        byte lowByte1 = Wire.read();
        byte highByte2 = Wire.read();
        byte lowByte2 = Wire.read();

        Status = word(highByte1, lowByte1);
        Status_wert = word(highByte2, lowByte2);
    }
    Serial.print("Slave empfangen(");
    Serial.print(Status);
    Serial.print(", ");
    Serial.print(Status_wert);
    Serial.println(") --  ");

    if (Status == 1)
        Einschalten();

    if (Status == 0)
        Ausschalten();

    if (Status == 2)
        Farbspektrum(Status_wert);

    if (Status == 3)
        Dimmmodus(Status_wert);

    if (Status == 4)
        whitemode();

    if (Status == 15)
        speed(Status_wert);

    if (Status == 9)
        Modi5(Status_wert);
}

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    randomSeed(analogRead(0)); // Das Werte immer zufällig sind wird wert von leeren Pin genutzt pin0

    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Wire.begin((uint8_t)I2C_DEV_ADDR);

    pixels.begin();
    pixels.clear();
    pixels.setBrightness(Brightness);
    pixels.show();
    delay(1000);
    Serial.println("------------------------------------------------------------------------------------");
}

void loop()
{
    if (Status == 5)
    {
        Modi1();
    }
    else if (Status == 6)
    {
        Modi2();
        if (Status != 6)
            return;
    }
    else if (Status == 7)
    {
        Modi3(15, 100);
        if (Status != 7)
            return;
    }
    else if (Status == 8)
    {
        Modi4(5, 64, true, random(10, 100));
        if (Status != 8)
            return;
    }
    /*if (Status == 9)
    {
        Modi5(Status_wert);
        if (Status != 9)
            return;
    }*/
    else if (Status == 0 && ausgeschalten == true && timer >= 20000) // Wenn Streifen aus und zufällig farben angezeigt wertden, schalte streifen aus
    {
        if (pixels.canShow())
        {
            pixels.clear();
            pixels.show();
        }
        Serial.println("schwarz kontrolliert");
        timer = 0;
    }
    else if (farbdarstellung == true && timer >= 20000)
    {
        if (pixels.canShow())
            pixels.show();
        timer = 0;
        Serial.println("Farbdarstellung kontrolliert");
    }
    // Status 8,9,10,11,12,3 ist für Modis
    // Serial.print(timer);
    // Serial.println(farbdarstellung);
    timer++; // aktualliesiert Streifen nach einiger Zeit.
    delay(1);
}

// 1: Einschalten
void Einschalten()
{
    Serial.println("Einschalten");
    Old_Status = Status;

    W = 255; // Weiß beim Einschalten
    R = 0;
    G = 0;
    B = 0;

    pixels.clear();
    pixels.setBrightness(Brightness);
    pixels.show();

    for (int j = 0; j < NUMPIXELS; j++)
    {
        pixels.setPixelColor(j, pixels.Color(R, G, B, W));
        pixels.show();
        delay(10);
        if (Status != 1)
            return;
    }
    ausgeschalten = false;
    farbdarstellung = false;
}

// 0: Ausschalten
void Ausschalten()
{
    Serial.println("Ausschalten");

    delay(100);
    Old_Status = Status;
    for (int j = NUMPIXELS; j >= 0; j--)
    {
        if (pixels.canShow())
        {
            pixels.setPixelColor(j, pixels.Color(0, 0, 0, 0));
            pixels.show();
        }
        delay(10);
        if (Status != 0)
            return;
    }
    R = 0;
    G = 0;
    B = 0;
    W = 0;
    ausgeschalten = true;
    farbdarstellung = false;
}

// 2: Farbspektrum
void Farbspektrum(int value)
{
    Serial.printf("Farbspecktrum: %d\n", value);
    Old_Status = Status;
    if (pixels.canShow())
    {
        pixels.fill(pixels.ColorHSV(value)); // Setzt pixel auf gewählte Farbe
        pixels.show();
    }

    uint32_t color = pixels.ColorHSV(value); // Dient um RGBW-Komponente aus Farbe zu extrahieren
    // Extrahieren der RGBW-Komponenten des Farbwerts
    W = (color >> 24) & 0xFF;
    R = (color >> 16) & 0xFF;
    G = (color >> 8) & 0xFF;
    B = color & 0xFF;

    farbdarstellung = true;
    /*
    delay(1000);
    pixels.fill(pixels.Color(R, G, B, W));
    pixels.show();

    Serial.print(R);
    Serial.print(" ");
    Serial.print(G);
    Serial.print(" ");
    Serial.print(B);
    Serial.print(" ");
    Serial.println(W); */
}

// 3: Dimmmodus
void Dimmmodus(int value)
{
    // Serial.println("Dimmmodus");
    Brightness = value;

    if (Brightness != last_Brightness) // Nur wenn sich wert ändert
    {
        if (pixels.canShow())
            pixels.setBrightness(Brightness);

        if (Old_Status != 5 && Old_Status != 6 && Old_Status != 7 && Old_Status != 8 && Old_Status != 9)
            pixels.show();
        delay(10);
    }

    /*if (Old_Status == 5)
        Status = 5;
    if (Old_Status == 6)
        Status = 6;
    if (Old_Status == 7)
        Status = 7;
    if (Old_Status == 8)
        Status = 8;
    if (Old_Status == 9)
        Status = 9;
    */

    for (int i = 5; i <= 13; i++)
    {
        if (Old_Status == i)
            Status = i;
    }

    if (Old_Status == 9)
        Status_wert = Old_Status_wert;

    last_Brightness = Brightness;
}

// 4: Setzt Farbe auf Weiß
void whitemode()
{
    Serial.println("whitemode");
    Old_Status = Status;

    R = 0;
    G = 0;
    B = 0;
    W = 255;
    if (pixels.canShow())
    {
        pixels.fill(pixels.Color(R, G, B, W));
        pixels.show();
    }
    farbdarstellung = true;
}

// 15: stellt die geschwindigkeiten der Animationen ein
void speed(int value)
{
    speed_value = value;

    /* if (Old_Status == 5)
         Status = 5;
     if (Old_Status == 6)
         Status = 6;
     if (Old_Status == 7)
         Status = 7;
     if (Old_Status == 8)
         Status = 8;
     if (Old_Status == 9)
         Status = 9; */

    for (int i = 5; i <= 13; i++)
    {
        if (Old_Status == i)
            Status = i;
    }

    if (Old_Status == 9)
        Status_wert = Old_Status_wert;
}

// 5: wechselnder Regenbogen
void Modi1()
{
    if (Status != 6 && Status != 7 && Status != 8 && Status != 9)
    {
        Serial.println("Modi1");
        Old_Status = Status;
        farbdarstellung = false;

        unsigned long previousMillis = 0;

        while (1 != 0)
        {
            for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256)
            {
                const long interval = 50; // speed_value;               // Verzögerungsgeschwindigkeit

                uint32_t color = pixels.ColorHSV(firstPixelHue); // Dient ums RGBW-Komponente aus Farbe zu extrahieren
                W = (color >> 24) & 0xFF;
                R = (color >> 16) & 0xFF;
                G = (color >> 8) & 0xFF;
                B = color & 0xFF;

                pixels.rainbow(firstPixelHue /*, 1, 255, Brightness*/);

                if (Status != 5 && Status != 3 && Status != 15)
                    return;

                // unsigned long currentMillis = millis();
                // if (currentMillis - previousMillis >= interval)
                //{
                //   previousMillis = currentMillis;
                delay(50);

                if (Status != 5 && Status != 3 && Status != 15)
                    return;

                if (pixels.canShow())
                    pixels.show();

                if (Status != 5 && Status != 3 && Status != 15)
                    return;
                // }
            }
            // pixels.clear();
            // Serial.println("fertig");
        }
        if (Status != 5 && Status != 3 && Status != 15)
            return;
    }
}

// 6: Springender Farbwechsel mit einer LED pause immer
void Modi2()
{
    if (Status != 5 && Status != 7 && Status != 8 && Status != 9)
    {
        Serial.println("Modi2");
        Old_Status = Status;
        farbdarstellung = false;

        int firstPixelHue = 0; // First pixel starts at red (hue 0)
        for (int a = 0; a < 30; a++)
        { // Repeat 30 times...
            for (int b = 0; b < 3; b++)
            {                   //'b' counts from 0 to 2...
                pixels.clear(); // Set all pixels in RAM to 0 (off)
                // 'c' counts up from 'b' to end of strip in increments of 3...
                for (int c = b; c < pixels.numPixels(); c += 3)
                {
                    int hue = firstPixelHue + c * 65536L / pixels.numPixels();
                    uint32_t color = pixels.gamma32(pixels.ColorHSV(hue)); // hue -> RGB
                    pixels.setPixelColor(c, color);                        // Set pixel 'c' to value 'color'

                    if (Status != 6 && Status != 3 && Status != 15) // Nur Modi2(5) und Dimmbar(3) wärendessen
                        return;
                }

                if (pixels.canShow())
                    pixels.show(); // Update strip with new contents

                delay(speed_value * 10);     // Pause for a moment
                firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames

                if (Status != 6 && Status != 3 && Status != 15) // Nur Modi2(5) und Dimmbar(3) wärendessen
                    return;
            }
            if (Status != 6 && Status != 3 && Status != 15) // Nur Modi2(5) und Dimmbar(3) wärendessen
                return;
        }
    }
}

// 7: Zufälliges aufleuchten von Sternen
void Modi3(int max_stars, int Steps)
{
    if (Status != 5 && Status != 6 && Status != 8 && Status != 9)
    {
        Serial.println("Modi3");
        Old_Status = Status;
        farbdarstellung = false;
        if (pixels.canShow())
        {
            pixels.clear();
            pixels.show();
        }

        R = 0;   // random(10,250);
        G = 0;   // random(10,250);
        B = 0;   // random(10,250);
        W = 255; // random(10, 250);

        int stars = random(10, max_stars); // Zufällige anzahl an sterne bestimmen
        int pos[stars];                    // jeweilige position des Starts

        float red, green, blue, white;

        int randpos;
        bool toclose;

        for (int i = 0; i <= (200 * random(0, 2)); i++)
        {
            delay(10);

            if (Status != 7 && Status != 3 && Status != 15)
                return;

            pixels.show();

            if (Status != 7 && Status != 3 && Status != 15)
                return;
        }

        // zufällige Position der Sterne bestimmen
        /*
            // zufällige Position der Sterne bestimmen
        for (int i = 0; i <= stars; i++)
        {
            pos[i] = random(0, NUMPIXELS);
        }*/
        for (int i = 0; i <= stars; i++)
        {
            do
            {
                randpos = random(0, NUMPIXELS); // Lege position für jeweiligen Stern fest
                toclose = false;

                // Überprüfe ob Pos zu Nah ist
                for (int m = 0; m < i; m++)
                {
                    if (abs(randpos - pos[m]) <= 1)
                    {
                        toclose = true;
                        break;
                    }
                }
            } while (toclose); // Wiederhole solange die Position zu nah ist

            pos[i] = randpos;
        }

        // Sterne aufhellen lassen
        for (int j = 0; j <= Steps; j++)
        {
            red = j * (R / Steps);
            green = j * (G / Steps);
            blue = j * (B / Steps);
            white = j * (W / Steps);

            // Sterne anzeigen
            for (int k = 0; k <= stars; k++)
            {
                pixels.setPixelColor(pos[k], red, green, blue, white);

                if (Status != 7 && Status != 3 && Status != 15)
                    return;
            }

            if (Status != 7 && Status != 3 && Status != 15)
                return;

            if (pixels.canShow())
                pixels.show();

            if (Status != 7 && Status != 3 && Status != 15)
                return;
            delay(10);
        }
        if (Status != 7 && Status != 3 && Status != 15)
            return;

        for (int i = 0; i <= (random(150, speed_value * 20) * random(1, 2)); i++)
        {
            if (Status != 7 && Status != 3 && Status != 15)
                return;

            delay(10);

            if (Status != 7 && Status != 3 && Status != 15)
                return;

            if (pixels.canShow())
                pixels.show();

            if (Status != 7 && Status != 3 && Status != 15)
                return;
        }

        // Sterne Abdunkeln lassen
        for (int m = Steps; m >= 0; m--)
        {
            red = m * (R / Steps);
            green = m * (G / Steps);
            blue = m * (B / Steps);
            white = m * (W / Steps);

            if (Status != 7 && Status != 3 && Status != 15)
                return;

            // Sterne anzeigen
            for (int l = 0; l <= stars; l++)
            {
                pixels.setPixelColor(pos[l], red, green, blue, white);

                if (Status != 7 && Status != 3 && Status != 15)
                    return;
            }
            if (Status != 7 && Status != 3 && Status != 15)
                return;

            if (pixels.canShow())
                pixels.show();

            if (Status != 7 && Status != 3 && Status != 15)
                return;
            delay(10);
        }

        if (Status != 7 && Status != 3 && Status != 15)
            return;

        pixels.clear();

        if (Status != 7 && Status != 3 && Status != 15)
            return;
        if (pixels.canShow())
            pixels.show();

        if (Status != 7 && Status != 3 && Status != 15)
            return;

        for (int j = 0; j <= (random(speed_value * 10, speed_value * 15 + 500) * random(1, 2)); j++)
        {
            delay(10);

            if (Status != 7 && Status != 3 && Status != 15)
                return;
        }
    }
}

// 8: Meteor-regen
void Modi4(byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay)
{
    if (Status != 5 && Status != 6 && Status != 7 && Status != 9)
    {
        Serial.println("Modi4");
        Old_Status = Status;
        farbdarstellung = false;

        byte color = random(255);
        pixels.clear();
        pixels.show();

        for (int i = 0; i < NUMPIXELS + NUMPIXELS + random(speed_value * 10, speed_value * 20); i++)
        {

            // fade brightness all LEDs one step
            for (int j = 0; j < NUMPIXELS; j++)
            {
                if ((!meteorRandomDecay) || (random(10) > 5))
                {
                    fadeToBlack(j, meteorTrailDecay);

                    if (Status != 8 && Status != 3 && Status != 15)
                        return;
                }

                if (Status != 8 && Status != 3 && Status != 15)
                    return;
            }

            // draw meteor
            for (int j = 0; j < meteorSize; j++)
            {
                if ((i - j < NUMPIXELS) && (i - j >= 0))
                {
                    pixels.setPixelColor(i - j, pixels.ColorHSV((color * 257)));

                    if (Status != 8 && Status != 3 && Status != 15)
                        return;
                }

                if (Status != 8 && Status != 3 && Status != 15)
                    return;
            }
            if (Status != 8 && Status != 3 && Status != 15)
                return;

            if (pixels.canShow())
                pixels.show();

            delay(SpeedDelay);

            if (Status != 8 && Status != 3 && Status != 15)
                return;
        }

        if (Status != 8 && Status != 3 && Status != 15)
            return;
    }
}

void fadeToBlack(int ledNo, byte fadeValue)
{

    // NeoPixel
    uint32_t oldColor;
    uint8_t r, g, b, w;

    oldColor = pixels.getPixelColor(ledNo);
    w = (oldColor & 0xff000000UL) >> 24;
    r = (oldColor & 0x00ff0000UL) >> 16;
    g = (oldColor & 0x0000ff00UL) >> 8;
    b = (oldColor & 0x000000ffUL);

    w = (w <= 10) ? 0 : (int)w - (w * fadeValue / 256);
    r = (r <= 10) ? 0 : (int)r - (r * fadeValue / 256);
    g = (g <= 10) ? 0 : (int)g - (g * fadeValue / 256);
    b = (b <= 10) ? 0 : (int)b - (b * fadeValue / 256);

    if (pixels.canShow())
        pixels.setPixelColor(ledNo, r, g, b);

    if (Status != 8 && Status != 3 && Status != 15)
        return;
}

// 9:Snake
void Modi5(int value)
{
    if (value == 0)
    {
        pixels.clear();
        pixels.show();
    }
    else
    {
        if (Status != 5 && Status != 6 && Status != 7 && Status != 8)
        {
            Serial.println("Modi5");
            Old_Status = Status;
            Old_Status_wert = Status_wert;

            pixels.clear();
            pixels.show();

            int8_t slave_delay = speed_value * NUMPIXELS;

            if (Status != 9 && Status != 3 && Status != 15)
                return;

            // int length = 0; // Länge der Snake

            // Delay für slaves
            /*for (int j = 0; j <= (NUMPIXELS * (value - 1) + 10); j++)
            {
                delay(speed_value);

                if (Status != 9 && Status != 3 && Status != 15)
                    return;

                // Serial.print(j);
                // Serial.println("delay Slave");
            }*/

            for (int i = 0; i <= NUMPIXELS; i++)
            {
                pixels.setPixelColor(i, pixels.Color(R, G, B, W));
                pixels.setPixelColor(i - 1, pixels.Color(0, 0, 0, 0));
                pixels.show();

                delay(speed_value);

                if (Status != 9 && Status != 3 && Status != 15)
                    return;

                // Serial.println("delay Pixel");
            }

            // delay Slaves
            /*for (int j = 0; j <= (NUMPIXELS * (8 - value)); j++) // Nach animation wartet slaves bis alle durch sind
            {
                delay(speed_value);

                if (Status != 9 && Status != 3 && Status != 15)
                    return;

                // Serial.print(j);
                // Serial.println("delay Slave");
            }*/

            if (Status != 9 && Status != 3 && Status != 15)
                return;
        }
    }
}

// ENDE