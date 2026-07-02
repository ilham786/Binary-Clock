/*
 * ============================================================
 *  BINARY CLOCK — Arduino Nano + MAX7219 + DS3231
 * ============================================================
 *  Author  : Robotics & Embedded Systems Project
 *  Version : 2.0
 *  Board   : Arduino Nano (ATmega328P)
 *
 *  DESCRIPTION:
 *    Reads Hours, Minutes, Seconds from a DS3231 RTC module
 *    and displays each value in binary (MSB top, LSB bottom)
 *    on a MAX7219 8×8 LED dot-matrix module.
 *
 *  DISPLAY LAYOUT (8 columns × 6 rows used):
 *    Columns 0-1 → Hours   (0–23, 5 bits)
 *    Columns 3-5 → Minutes (0–59, 6 bits)
 *    Columns 6-7 → Seconds (0–59, 6 bits used across 2 cols)
 *
 *    Simplified 3-group layout (each group = one value):
 *      Col 0 = Hours  tens digit in BCD (max 2, 2 bits)
 *      Col 1 = Hours  units digit in BCD (max 9, 4 bits)
 *      Col 2 = separator (off)
 *      Col 3 = Minutes tens digit in BCD (max 5, 3 bits)
 *      Col 4 = Minutes units digit in BCD (max 9, 4 bits)
 *      Col 5 = separator (off)
 *      Col 6 = Seconds tens digit in BCD (max 5, 3 bits)
 *      Col 7 = Seconds units digit in BCD (max 9, 4 bits)
 *
 *    Each column shows bits top-to-bottom: bit3 bit2 bit1 bit0
 *    (rows 4–7; rows 0–3 are the upper half showing bit values)
 *
 *  WIRING:
 *    MAX7219  →  Arduino Nano
 *    VCC      →  5V
 *    GND      →  GND
 *    DIN      →  D11 (MOSI)
 *    CLK      →  D13 (SCK)
 *    CS/LOAD  →  D10
 *
 *    DS3231   →  Arduino Nano
 *    VCC      →  3.3V
 *    GND      →  GND
 *    SDA      →  A4
 *    SCL      →  A5
 *
 *  LIBRARIES (install via Arduino IDE Library Manager):
 *    • MD_MAX72XX  by MajicDesigns  (v3.x)
 *    • RTClib      by Adafruit      (v2.x)
 *
 * ============================================================
 */

#include <Wire.h>
#include <RTClib.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// ── Hardware Config ─────────────────────────────────────────
#define HARDWARE_TYPE  MD_MAX72XX::FC16_HW   // Change to GENERIC_HW if FC16 doesn't work
#define MAX_DEVICES    1                      // Number of chained MAX7219 modules
#define CS_PIN         10                     // Chip Select / LOAD pin

// ── Brightness (0=dim … 15=max) ─────────────────────────────
#define BRIGHTNESS     5

// ── Row mapping: which matrix row = which bit weight ────────
//    Row 0 (top)    = bit 3  (value 8)
//    Row 1          = bit 2  (value 4)
//    Row 2          = bit 1  (value 2)
//    Row 3 (bottom) = bit 0  (value 1)
//    Rows 4-7 are unused (kept dark)

// ── Objects ─────────────────────────────────────────────────
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
RTC_DS3231  rtc;

// ── State ───────────────────────────────────────────────────
uint8_t lastSecond = 255;   // forces first draw immediately

// ============================================================
//  setBCDColumn()
//  Lights up a single column to represent a BCD digit (0–9)
//  col  : matrix column (0 = leftmost)
//  val  : value 0–9 (only lower 4 bits used)
// ============================================================
void setBCDColumn(uint8_t col, uint8_t val) {
  // Build an 8-bit column byte.
  // Bit 7 = topmost LED (row 0). We use rows 4–7 for bits 3-0.
  // Rows 0-3 show the bit-weight labels (always off in this sketch).
  uint8_t colByte = 0x00;

  // Map nibble bits to lower 4 rows of the 8-row column
  if (val & 0x08) colByte |= (1 << 4);   // bit3 → row 4
  if (val & 0x04) colByte |= (1 << 5);   // bit2 → row 5
  if (val & 0x02) colByte |= (1 << 6);   // bit1 → row 6
  if (val & 0x01) colByte |= (1 << 7);   // bit0 → row 7 (bottom LED)

  mx.setColumn(col, colByte);
}

// ============================================================
//  displayTime()
//  Converts H:M:S → BCD digits, writes to matrix columns
// ============================================================
void displayTime(uint8_t h, uint8_t m, uint8_t s) {
  mx.clear();

  // BCD decomposition
  uint8_t hT = h / 10;   // Hours tens   (0-2)
  uint8_t hU = h % 10;   // Hours units  (0-9)
  uint8_t mT = m / 10;   // Minutes tens (0-5)
  uint8_t mU = m % 10;   // Minutes units(0-9)
  uint8_t sT = s / 10;   // Seconds tens (0-5)
  uint8_t sU = s % 10;   // Seconds units(0-9)

  // Write columns (col 2 and col 5 are separators → left dark)
  setBCDColumn(0, hT);
  setBCDColumn(1, hU);
  // col 2 = separator (skip)
  setBCDColumn(3, mT);
  setBCDColumn(4, mU);
  // col 5 = separator (skip)
  setBCDColumn(6, sT);
  setBCDColumn(7, sU);
}

// ============================================================
//  flashError()
//  Blinks all LEDs if RTC is missing / not found
// ============================================================
void flashError() {
  while (true) {
    mx.clear();
    delay(300);
    for (uint8_t c = 0; c < 8; c++) mx.setColumn(c, 0xFF);
    delay(300);
  }
}

// ============================================================
//  setup()
// ============================================================
void setup() {
  Serial.begin(9600);
  Serial.println(F("Binary Clock v2.0 — Starting..."));

  // Init MAX7219
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, BRIGHTNESS);
  mx.control(MD_MAX72XX::UPDATE,    MD_MAX72XX::ON);
  mx.clear();

  // Init RTC
  if (!rtc.begin()) {
    Serial.println(F("ERROR: DS3231 RTC not found! Check wiring."));
    flashError();
  }

  // If RTC lost power (first-time setup), set to compile time
  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power — setting to compile time."));
    // Sets RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println(F("RTC OK. Binary clock running."));

  // Startup animation: sweep columns
  for (uint8_t i = 0; i < 8; i++) {
    mx.setColumn(i, 0xFF);
    delay(60);
  }
  for (uint8_t i = 0; i < 8; i++) {
    mx.setColumn(i, 0x00);
    delay(60);
  }
}

// ============================================================
//  loop()
// ============================================================
void loop() {
  DateTime now = rtc.now();

  // Only redraw when second changes (reduces flicker)
  if (now.second() != lastSecond) {
    lastSecond = now.second();

    displayTime(now.hour(), now.minute(), now.second());

    // Debug output to Serial Monitor
    char buf[20];
    snprintf(buf, sizeof(buf), "Time: %02d:%02d:%02d",
             now.hour(), now.minute(), now.second());
    Serial.println(buf);
  }

  delay(100);   // Poll 10× per second; lightweight on CPU
}

// ============================================================
//  HOW TO SET A CUSTOM TIME:
//  Replace the rtc.lostPower() block with:
//
//    rtc.adjust(DateTime(2025, 6, 15,  14, 30, 0));
//                        YYYY  MM  DD  HH  MM  SS
//
//  Upload once, then comment out and re-upload.
// ============================================================
