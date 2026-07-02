# ⏱ Binary Clock
### Arduino Nano + MAX7219 LED Matrix + DS3231 RTC

> A real-time binary clock that displays Hours, Minutes, and Seconds in BCD binary format on an 8×8 LED dot-matrix display — driven by an accurate DS3231 real-time clock module.

---

## 📸 Preview

```
Matrix Layout (time = 14:35:52)
────────────────────────────────────────────────
 BIT  │ H₁₀ H₁  │  M₁₀  M₁  │  S₁₀  S₁
──────┼──────────┼────────────┼────────────
  8   │  ○   ○  │   ○    ○  │   ○    ○
  4   │  ○   ●  │   ○    ●  │   ●    ○
  2   │  ○   ○  │   ●    ○  │   ○    ●
  1   │  ●   ○  │   ●    ●  │   ○    ○
──────┼──────────┼────────────┼────────────
 DEC  │  1   4  │   3    5  │   5    2
────────────────────────────────────────────────
  ●  = LED ON (bit = 1)     ○ = LED OFF (bit = 0)
  Each group = BCD digit.   Read top→bottom = 8,4,2,1
```

---

## 🧰 Components List

| # | Component | Qty | Notes |
|---|-----------|-----|-------|
| 1 | Arduino Nano | 1 | ATmega328P, 5V, 16 MHz |
| 2 | MAX7219 8×8 LED Matrix Module | 1 | FC-16 type (common eBay module) |
| 3 | DS3231 RTC Module | 1 | With CR2032 coin cell battery |
| 4 | CR2032 Coin Cell Battery | 1 | Keeps time when unpowered |
| 5 | Breadboard (half or full size) | 1 | For prototyping |
| 6 | Jumper Wires (M-M) | ~12 | Dupont type |
| 7 | 10µF Electrolytic Capacitor | 1 | Across VCC/GND of MAX7219 |
| 8 | USB-A to Mini-USB Cable | 1 | For programming the Nano |
| 9 | 5V USB Power Supply | 1 | For standalone use |

**Optional (for final build):**
- Soldering iron + solder
- PCB or perfboard
- 3D-printed or laser-cut enclosure
- Push button (for brightness control or time-set)

---

## ⚡ Wiring

### MAX7219 LED Matrix → Arduino Nano (SPI)

```
MAX7219 Pin    Arduino Nano Pin    Wire Color
───────────    ────────────────    ──────────
VCC       →   5V                  Red
GND       →   GND                 Black
DIN       →   D11 (MOSI)          Green
CLK       →   D13 (SCK)           Blue
CS / LOAD →   D10                 Yellow
```

> ⚠️ Add a **10µF capacitor** between VCC and GND pins on the MAX7219 module. This prevents brownout glitches when many LEDs light up at once.

### DS3231 RTC → Arduino Nano (I²C)

```
DS3231 Pin    Arduino Nano Pin    Wire Color
──────────    ────────────────    ──────────
VCC       →   3.3V                Red
GND       →   GND                 Black
SDA       →   A4                  Orange
SCL       →   A5                  Orange
```

> ⚠️ **CRITICAL:** Connect DS3231 VCC to **3.3V**, NOT 5V. The DS3231 is a 3.3V device. Connecting to 5V will damage it permanently.

---

## 📦 Software Setup

### Step 1 — Install Arduino IDE
Download from: https://www.arduino.cc/en/software

### Step 2 — Install Required Libraries
Open Arduino IDE → **Sketch → Include Library → Manage Libraries...**

Search and install:

| Library | Author | Version |
|---------|--------|---------|
| `MD_MAX72XX` | MajicDesigns | ≥ 3.3.1 |
| `MD_Parola` | MajicDesigns | ≥ 3.6.2 |
| `RTClib` | Adafruit | ≥ 2.1.1 |

> `Wire.h` and `SPI.h` are built into Arduino IDE — no installation needed.

### Step 3 — Select Board & Port
- **Tools → Board → Arduino AVR Boards → Arduino Nano**
- **Tools → Processor → ATmega328P (Old Bootloader)** ← Try this if upload fails
- **Tools → Port → COMx** (Windows) or `/dev/ttyUSB0` (Linux/Mac)

### Step 4 — Upload the Sketch
Open `binary_clock.ino` and click **Upload (→)**.

On first boot with a fresh DS3231 battery, the sketch auto-sets the RTC to the **compile time** of the sketch. This is accurate to within a few seconds.

### Step 5 — Verify on Serial Monitor
Open **Tools → Serial Monitor** at **9600 baud**. You should see:
```
Binary Clock v2.0 — Starting...
RTC OK. Binary clock running.
Time: 14:35:52
Time: 14:35:53
...
```

---

## 🧠 How the Binary Display Works

The display uses **Binary Coded Decimal (BCD)** format. Each decimal digit (0–9) is independently converted to a 4-bit binary number and shown as a column of 4 LEDs:

```
Bit Weight:   8  4  2  1
              ↓  ↓  ↓  ↓
Digit 5:      0  1  0  1   →  4+1 = 5  ✓
Digit 9:      1  0  0  1   →  8+1 = 9  ✓
Digit 0:      0  0  0  0   →  0       ✓
```

**Reading the clock:**
1. Each column of 4 LEDs represents one decimal digit
2. Read top → bottom: 8 → 4 → 2 → 1
3. Add up the weights of all lit LEDs in that column
4. Result = that digit's value

**Column assignment on the 8×8 matrix:**

| Column | Represents |
|--------|-----------|
| 0 | Hours tens (0–2) |
| 1 | Hours units (0–9) |
| 2 | Separator (off) |
| 3 | Minutes tens (0–5) |
| 4 | Minutes units (0–9) |
| 5 | Separator (off) |
| 6 | Seconds tens (0–5) |
| 7 | Seconds units (0–9) |

---

## 💻 Code Architecture

```
binary_clock.ino
│
├── setup()
│   ├── mx.begin()          → Init MAX7219
│   ├── rtc.begin()         → Init DS3231 via I²C
│   ├── rtc.lostPower()     → Auto-set time if battery died
│   └── Startup animation   → LED sweep effect
│
└── loop()
    ├── rtc.now()           → Read current H:M:S
    ├── [if second changed]
    │   ├── displayTime()   → Main display function
    │   └── Serial.println  → Debug output
    └── delay(100)          → Poll 10× per second
        │
        └── displayTime(h, m, s)
            ├── mx.clear()
            ├── BCD decomposition (tens / units)
            └── setBCDColumn() × 6  → Write each column
```

---

## ⚙️ Configuration

In `binary_clock.ino`, you can adjust these constants:

```cpp
// MAX7219 hardware variant
#define HARDWARE_TYPE  MD_MAX72XX::FC16_HW   // FC16 modules (most common)
// Alternative: MD_MAX72XX::GENERIC_HW       // Bare MAX7219 chips

// Number of chained MAX7219 modules
#define MAX_DEVICES  1   // Increase if you chain multiple modules

// CS pin for MAX7219
#define CS_PIN  10   // Change if you use a different pin

// Display brightness (0 = dimmest, 15 = brightest)
#define BRIGHTNESS  5
```

### Setting a Custom Time Manually

Replace the `lostPower()` block in `setup()`:

```cpp
// Set to June 15, 2025 at 14:30:00
rtc.adjust(DateTime(2025, 6, 15, 14, 30, 0));
```

Upload once with this line, then comment it out and re-upload. The DS3231's battery keeps time even when the Arduino is unplugged.

---

## 🔧 Troubleshooting

| Problem | Likely Cause | Fix |
|---------|-------------|-----|
| Nothing lights up | Wiring error or wrong hardware type | Check DIN→D11, CLK→D13, CS→D10. Try `GENERIC_HW` |
| All LEDs flash repeatedly | RTC not found | Check SDA→A4, SCL→A5. Confirm 3.3V power to RTC |
| Time is wrong | RTC was set to compile time | Set a custom time (see above) |
| Time drifts fast | No battery in DS3231 | Insert CR2032 cell |
| Upload fails | Wrong bootloader | Try Old Bootloader in Tools menu |
| Flickering LEDs | Power supply noise | Add 10µF cap across VCC/GND on MAX7219 |
| Matrix is upside-down | Different module rotation | Change `HARDWARE_TYPE` to `GENERIC_HW` or rotate module |

---

## 🚀 Enhancements & Next Steps

### Easy
- **Brightness control** — Pot on A0 → `mx.control(MD_MAX72XX::INTENSITY, map(analogRead(A0), 0, 1023, 0, 15))`
- **12/24hr toggle** — Add a button on any digital pin

### Medium
- **Chain 2 modules** — Show seconds on a second 8×8 matrix for larger display
- **Alarm** — Use DS3231's built-in alarm output (SQW pin)
- **Colon blink** — Blink the separator columns on/off each second

### Advanced
- **OLED companion display** — Show decimal time on 128×64 OLED alongside binary matrix
- **WiFi sync (ESP8266/ESP32)** — Replace Arduino Nano, sync time from NTP server
- **Custom PCB** — Design in KiCad, order from JLCPCB or PCBWay
- **3D-printed enclosure** — Acrylic diffuser panel over the LED matrix

---

## 📁 Project Files

```
binary_clock/
├── binary_clock.ino       ← Arduino sketch (main code)
├── circuit_diagram.html   ← Interactive circuit diagram
├── simulation.html        ← Live browser simulation of the clock
├── credits.html           ← Developer credits and technology attribution
└── README.md              ← This file
```

---

## Credits

**Designed & Developed by**<br>
**Ilham Farooque**

Ilham Farooque is credited for:

- Complete application development
- System architecture
- Circuit diagrams
- Source code
- UI/UX Design
- Database design
- Technical documentation
- Full technology stack implementation
- Project planning
- Development and maintenance

### Developer Profile

**Ilham Farooque**<br>
**Full Stack Developer**

Passionate about building scalable applications, clean architectures, intuitive user experiences, modern web technologies, and innovative software solutions.

### Social Links

- GitHub: https://github.com/ilham786
- X (Twitter): https://x.com/IlhamFarooque
- Instagram: https://www.instagram.com/ilham_farooque/

### Technologies Used

- HTML5
- CSS3
- JavaScript
- SVG
- Arduino
- C/C++
- MAX7219
- DS3231 RTC
- MD_MAX72XX
- RTClib
- Wire
- SPI
- Git
- GitHub

Every line of code, circuit design, and technical implementation in this project has been crafted and maintained by Ilham Farooque.

---

## 📄 License

MIT License — free to use, modify, and share. Attribution appreciated.

---

## 🙏 Libraries & Credits

- **[MD_MAX72XX](https://github.com/MajicDesigns/MD_MAX72XX)** by MajicDesigns — MAX7219 driver
- **[RTClib](https://github.com/adafruit/RTClib)** by Adafruit — DS3231 RTC library
- Arduino community — for the incredible open-source ecosystem

---

*Built with ❤️ for makers, hardware enthusiasts, and anyone who wants to tell time the hard way.*
