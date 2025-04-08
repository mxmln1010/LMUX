// Name:       Peter Thieme
// Projekt:    LED_Grünler
// Controler:  ESP32 U32 WROOM
// Board:      DOIT ESP32 DEVKIT V1
// Info:
//-----------------------------------------//

#include <Arduino.h>
#include "Wire.h"
#include "header.h"
#include <ESP32SPISlave.h> //version 0.3.0

//-----ESP32 als Slave-----
ESP32SPISlave slave;
static constexpr uint8_t BUFFER_SIZE{12}; // static constexpr uint32_t BUFFER_SIZE{ 12 };
uint8_t spi_slave_tx_buf[BUFFER_SIZE];    // speichert jeweils 1 byte Code Info
uint8_t spi_slave_rx_buf[BUFFER_SIZE];    // speichert jeweils 1 Byte Code info

uint8_t code_byte[5];     // Code zur weiterverarbeitung
uint8_t old_code_byte[5]; // Damit nur Code nur einmal gesendet wird
//----------------------------

//---------I2C----------
//                       Gruppen =    0    0      1     1     2     2     3
const uint8_t slave_addresses[7] = {0x15, 0x25, 0x35, 0x45, 0x11, 0x22, 0x33}; // Adressen der Slaves
const uint8_t num_slaves = sizeof(slave_addresses) / sizeof(slave_addresses[0]);
int8_t ani_end;                  // ob animation zu ende ist
bool zustand_slaves[num_slaves]; // ISt jeweilgier Slave in gesuchter Animation
int wert1, wert2;
//----------------------------

//----------------LED_Steuerung----------
bool strip1 = false; // Zustand des jeweiligen strips AN/AUS
bool strip2 = false;
bool strip3 = false;
bool strip4 = false;
bool stripall = false;

int8_t speed_state = 10; // welcher mod gesendet wird bei tasten s- bzw s+

uint8_t anzahl_modi = 4;    // bei veränderung der anzhal der modi auch ändern // mit Modi 9 (snake) anzahl = 5
uint8_t aktueller_modi = 0; // aktueller Modi

uint8_t aktueller_dimmwert[7]{5, 10, 25, 50, 100, 200, 250};
uint8_t dimm_value = 1;
uint8_t last_byte = 100;

// Modi5 (snake)
int8_t transmission_delay; // modi9 (snake) nächste transmission abwarten
int8_t number = 1;         // für Modi9 (snake) wann nächste Transmission kommt
int8_t modi5_step = 0;     // im loop hochzählen

int modi5_strips[7]{0, 0, 0, 0, 0, 0, 0}; // Vektor für strips in reihenfolge
//-------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  randomSeed(analogRead(0)); // Das Werte immer zufällig sind wird wert von leeren Pin genutzt

  // I2C
  Wire.begin();

  // ESP32Slave
  slave.setDataMode(SPI_MODE1);
  slave.begin();
  memset(spi_slave_tx_buf, 0, BUFFER_SIZE);
  memset(spi_slave_rx_buf, 0, BUFFER_SIZE);

  Serial.println("Start des Systems");
  // delay(1000);
  Transmission(3, 50, 0, 7); // Helligkeit einstellen
  delay(100);
  Transmission(15, 10, 0, 7); // Speed einstellen
  delay(100);
  // Transmission(1, 0, 0, 7); // Einschalten
  // delay(1000);
  Transmission(0, 0, 0, 7); // Ausschalten
  delay(1000);
}

void loop()
{
  SPI_code();

  if (code_byte[2] != old_code_byte[2])
  {
    // Serial.println(code_byte[0], HEX);
    // Serial.println(code_byte[1], HEX);
    // Serial.println(code_byte[2], HEX);
    // Serial.println(code_byte[3], HEX);
    // Serial.println(code_byte[4], HEX);
    // Serial.println(code_byte[5], HEX);

    vergleich(code_byte[0], code_byte[1], code_byte[3]); // Vergleicht den Code mit Tastencodes
  }
  old_code_byte[2] = code_byte[2];

  if (code_byte[0] == 0xA3)
  {
    if (5 + abs(aktueller_modi) == 9 && transmission_delay == 80)
    {
      if (modi5_strips[modi5_step] > 0)
      {
        if (code_byte[0] != 0xA3)
          return;
        Transmission(9, 1, modi5_strips[modi5_step] - 1, modi5_strips[modi5_step] - 1);
      }
      modi5_step++;
      transmission_delay = 0;

      if (modi5_step > 7)
        modi5_step = 0;
    }
    else if (5 + abs(aktueller_modi) == 9)
    {
      // Serial.print("timer");
      transmission_delay++;
      delay(speed_state);
    }
  }

  delay(10); // TODO: IST NEU
}

void Transmission(int inhalt1, int inhalt2, uint8_t streifen_von, uint8_t streifen_bis)
{
  Serial.print("Slave ");
  for (int i = streifen_von; i <= streifen_bis; i++)
  {
    Wire.beginTransmission(slave_addresses[i]);
    Wire.write(highByte(inhalt1)); // Sendet höheres byte von inhalt1
    Wire.write(lowByte(inhalt1));  // Sendet Niedrigeres BYte von inhalt1

    Wire.write(highByte(inhalt2)); // Sendet höheres byte von inhalt1
    Wire.write(lowByte(inhalt2));  // Sendet Niedrigeres BYte von inhalt1
    Wire.endTransmission();

    Serial.printf("%d", i);
    Serial.print(" ");
  }
  Serial.print("gesteuert mit Inhalt: ");
  Serial.print(inhalt1);
  Serial.print("  ");
  Serial.println(inhalt2);
}

// scannt Controller auf Informationen von Fernbedienung
void SPI_code()
{
  slave.wait(spi_slave_rx_buf, NULL, BUFFER_SIZE);

  while (slave.available())
  {

    if ((spi_slave_rx_buf[0]) == 0x32 && spi_slave_rx_buf[1] == 0x9 && spi_slave_rx_buf[2] == 0xEE && spi_slave_rx_buf[3] == 0xB9 && spi_slave_rx_buf[4] == 0x88 && (spi_slave_rx_buf[5]) == 0x34)
    {
      // print_code(); // Zeigt den Code im terminal

      for (int i = 0; i <= 5; i++)
      {
        code_byte[i] = spi_slave_rx_buf[6 + i];
      }
    }
    slave.pop();
  }
}

// printed alle Codes
void print_code()
{
  Serial.print(spi_slave_rx_buf[0], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[1], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[2], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[3], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[4], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[5], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[6], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[7], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[8], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[9], HEX);
  Serial.print("  ");
  Serial.print(spi_slave_rx_buf[10], HEX);
  Serial.print("  ");
  Serial.println(spi_slave_rx_buf[11], HEX);
}

void vergleich(uint8_t byte6, uint8_t byte7, uint8_t byte9)
{
  //                        0   1   2   3   4   5     code_byte
  //  0  1  2   3   4   5   6   7   8   9   10  11    spi_slave_rx_buf
  // 32  9  EE  B9  88  34  9E  70  13  5D  D1  67    CODE

  // Ein-und Ausschalten
  {
    // Einschalten: ALLE
    if (byte6 == 0x9E && byte7 == 0x70 /*&& byte9 == 0x5D*/)
    {
      if (strip1 == false)
      {
        Transmission(1, 0, 0, 1);
        strip1 = true;
      }
      if (strip2 == false)
      {
        Transmission(1, 0, 2, 3);
        strip2 = true;
      }
      if (strip3 == false)
      {
        Transmission(1, 0, 4, 5);
        strip3 = true;
      }
      if (strip4 == false)
      {
        Transmission(1, 0, 6, 6);
        strip4 = true;
      }
      // Serial.println("delay");
      delay(500);
    }
    // Ausschalten: ALLE
    if (byte6 == 0x9E && byte7 == 0x79 /*&& byte9 == 0x5D*/)
    {
      Transmission(0, 0, 0, 7);
      strip1 = false;
      strip2 = false;
      strip3 = false;
      strip4 = false;
      stripall = false;
      // Serial.println("delay");
      delay(500);
    }
    // Einschalten: Sigment 1
    if (byte6 == 0x9E && byte7 == 0x71 && byte9 == 0x5E)
    {
      if (strip1 == false)
      {
        Transmission(1, 0, 0, 1);
        strip1 = true;
        delay(500);
      }
    }
    // Ausschalten: Sigment 1
    if (byte6 == 0x9E && byte7 == 0x7A /*&& byte9 == 0x5E*/)
    {
      if (strip1 == true)
      {
        Transmission(0, 0, 0, 1);
        strip1 = false;
        stripall = false;
        delay(500);
      }
    }
    // Einschalten: Sigment 2
    if (byte6 == 0x9E && byte7 == 0x72 && byte9 == 0x5F)
    {
      if (strip2 == false)
      {
        Transmission(1, 0, 2, 3);
        strip2 = true;
        delay(500);
      }
    }
    // Ausschalten: Sigment 2
    if (byte6 == 0x9E && byte7 == 0x7B /*&& byte9 == 0x5F*/)
    {
      if (strip2 == true)
      {
        Transmission(0, 0, 2, 3);
        strip2 = false;
        stripall = false;
        delay(500);
      }
    }
    // Einschalten: Sigment 3
    if (byte6 == 0x9E && byte7 == 0x73 && byte9 == 0x60)
    {
      if (strip3 == false)
      {
        Transmission(1, 0, 4, 5);
        strip3 = true;
        delay(500);
      }
    }
    // Ausschalten: Sigment 3
    if (byte6 == 0x9E && byte7 == 0x7C /*&& byte9 == 0x60*/)
    {
      if (strip3 == true)
      {
        Transmission(0, 0, 4, 5);
        strip3 = false;
        stripall = false;
        delay(500);
      }
    }
    // Einschalten: Sigment 4
    if (byte6 == 0x9E && byte7 == 0x74 && byte9 == 0x61)
    {
      if (strip4 == false)
      {
        Transmission(1, 0, 6, 6);
        strip4 = true;
        delay(500);
      }
    }
    // Ausschalten: Sigment 4
    if (byte6 == 0x9E && byte7 == 0x7D /*&& byte9 == 0x61*/)
    {
      if (strip4 = true)
      {
        Transmission(0, 0, 6, 6);
        strip4 = false;
        stripall = false;
        delay(500);
      }
    }
  }

  // Buttons
  {
    // s-   Soll den Speed wechseln
    if (byte6 == 0x9E && byte7 == 0x83)
    {
      speed_state = speed_state + 10;
      /*  if (speed_state >= 50)
          speed_state = 50;
          */

      speed_state = (speed_state >= 50) ? 50 : speed_state;

      find_transmission(byte9, 15, speed_state);
    }

    // s+ Soll den Speed wechseln
    if (byte6 == 0x9E && byte7 == 0x82)
    {
      speed_state = speed_state - 10;
      /* if (speed_state <= 5)
         speed_state = 5; */

      speed_state = (speed_state <= 5) ? 5 : speed_state;
      find_transmission(byte9, 15, speed_state);
    }

    // w  Soll Licht auf Weiß stellen
    if (byte6 == 0x9E && byte7 == 0x84)
    {
      find_transmission(byte9, 4, 0);
    }

    // M Ändert Modi
    if (byte6 == 0xA3) // byte7 geht zwischen 0x70 und 0x78
    {                  // Serial.println(round(map(byte7, 0x70, 0x78, 0, 8)));

      aktueller_modi++;
      aktueller_modi = (aktueller_modi >= anzahl_modi) ? 0 : aktueller_modi;

      find_transmission(byte9, 5 + abs(aktueller_modi), 0);

      if (5 + abs(aktueller_modi) == 9) // Wenn Modi 9 dann erst nachgfragen wer alles dabei ist von slaves
      {
        transmission_delay = 80;
        modi5_transmission(byte9);
        Serial.println("M: Modi5");
      }
    }

    // Farbmischungen Also oben rechts knopf und Rad
    if (byte6 == 0x9F)
    {
      find_transmission(byte9, 2, round(map(byte7 + 0x88, 0, 255, 0, 65536)));
    }

    // Aufhellen/Dimmen
    if (byte6 == 0xA2) // byte7 geht zwischen 0x70 und 0xD4
    {
      if (byte7 > last_byte || byte7 == 0xD4)
        dimm_value++;
      if (byte7 < last_byte || byte7 == 0x70)
        dimm_value--;

      dimm_value = (dimm_value <= 1) ? 1 : dimm_value;
      dimm_value = (dimm_value >= 7) ? 7 : dimm_value;

      find_transmission(byte9, 3, aktueller_dimmwert[dimm_value - 1]);

      last_byte = byte7;
    }

    // uint8_t akuteller_dimmwert;
    // uint8_t last_dimmwert;

    // O+ / O- Keine Funktion
    // if (byte6 == 0xA4)
    //{ // 0xD4 bis 0x70
    // find_transmission(byte9, map(byte7, 0xD4, 0x70, 5, 8), 0);
    //}
    // Sdsad
  }

  if (strip1 == true && strip2 == true && strip3 == true && strip4 == true)
    stripall = true;
  if (strip1 == false && strip2 == false && strip3 == false && strip4 == false)
    stripall = false;
}

void find_transmission(uint8_t byte, int value1, int value2)
{
  if (byte == 0x5D && stripall == true) // Steuern ALLE
    Transmission(value1, value2, 0, 7);

  else
  {
    if (byte == 0x5D && strip1 == true) // Steuern Sigment 1 Wenn noch auf ALL
      Transmission(value1, value2, 0, 1);
    if (byte == 0x5D && strip2 == true) // Steuern Sigment 2 Wenn noch auf ALL
      Transmission(value1, value2, 2, 3);
    if (byte == 0x5D && strip3 == true) // Steuern Sigment 3 Wenn noch auf ALL
      Transmission(value1, value2, 4, 5);
    if (byte == 0x5D && strip4 == true) // Steuern Sigment 4 Wenn noch auf ALL
      Transmission(value1, value2, 6, 6);
  }
  if (byte == 0x5E && strip1 == true) // Steuern Sigment 1
    Transmission(value1, value2, 0, 1);
  if (byte == 0x5F && strip2 == true) // Steuern Sigment 2
    Transmission(value1, value2, 2, 3);
  if (byte == 0x60 && strip3 == true) // Steuern Sigment 3
    Transmission(value1, value2, 4, 5);
  if (byte == 0x61 && strip4 == true) // Steuern Sigment 4
    Transmission(value1, value2, 6, 6);

  delay(10);
}

// Modi9 (Snake) jeweiligen Strip zuordnen
void modi5_transmission(uint8_t byte)
{ // lege auf modi5_vektor fest welcher strips welche pos hat
  int8_t number = 1;

  for (int i = 0; i < 7; i++) // den vektor NULLEN
    modi5_strips[i] = 0;

  if (byte == 0x5D && stripall == true) // Steuern ALLE
  {
    for (int i = 0; i < 7; i++) // Sendet an jeden Slaves den Modi mit der dazugehörigen Position in der snake schleife
    {
      modi5_strips[i] = i + 1;
    }
  }

  {                                     // Steuern der Sigmente wenn All noch true
    if (byte == 0x5D && strip1 == true) // Steuern Sigment 1 Wenn noch auf ALL
    {
      modi5_strips[0] = number;
      number++;
      modi5_strips[1] = number;
      number++;
    }

    if (byte == 0x5D && strip2 == true) // Steuern Sigment 2 Wenn noch auf ALL
    {
      modi5_strips[2] = number;
      number++;
      modi5_strips[3] = number;
      number++;
    }

    if (byte == 0x5D && strip3 == true) // Steuern Sigment 3 Wenn noch auf ALL
    {
      modi5_strips[4] = number;
      number++;
      modi5_strips[5] = number;
      number++;
    }

    if (byte == 0x5D && strip4 == true) // Steuern Sigment 4 Wenn noch auf ALL
    {
      modi5_strips[6] = number;
      number++;
    }
  }

  {                                     // Einzelne Sigmente ansteuern wenn All off
    if (byte == 0x5E && strip1 == true) // Steuern Sigment 1
    {
      modi5_strips[0] = number;
      number++;
      modi5_strips[1] = number;
      number++;
    }

    if (byte == 0x5F && strip2 == true) // Steuern Sigment 2
    {
      modi5_strips[2] = number;
      number++;
      modi5_strips[3] = number;
      number++;
    }

    if (byte == 0x60 && strip3 == true) // Steuern Sigment 3
    {
      modi5_strips[4] = number;
      number++;
      modi5_strips[5] = number;
      number++;
    }

    if (byte == 0x61 && strip4 == true) // Steuern Sigment 4
    {
      modi5_strips[6] = number;
      number++;
    }
  }
  Serial.println(modi5_strips[0]);
  Serial.println(modi5_strips[1]);
  Serial.println(modi5_strips[2]);
  Serial.println(modi5_strips[3]);
  Serial.println(modi5_strips[4]);
  Serial.println(modi5_strips[5]);
  Serial.println(modi5_strips[6]);

  Serial.println("Modi5transmssion ende");
  // ENDE
}

// END