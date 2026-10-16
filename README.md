# Voice Trainer Pendant

The Voice Trainer is an open-source wearable device designed to monitor vocal pitch and deliver real-time haptic feedback when pitch drops below a target threshold. 

The system uses a bone-conduction MEMS microphone pressed against the neck to isolate vocal cord vibrations from ambient acoustic noise, processes the signal locally on an ESP32-S3 microcontroller using the YIN pitch-tracking algorithm, and drives an ERM coin vibration motor via an I2C haptic driver.

---

## Features

- **Bone-Conduction Sensing:** Utilizes a PUI Audio VMM-1627L-R contact microphone positioned on the neck to eliminate ambient room noise and acoustic cross-talk.
- **Low-Latency Pitch Tracking:** Implements the autocorrelation-based YIN algorithm running directly on the ESP32-S3 dual-core processor.
- **Configurable Haptics:** Powered by an Adafruit DRV2605L driver and an ERM coin motor to deliver distinct tactile clicks and buzzes.
- **Low-Profile Enclosure:** Two-piece 3D-printed enclosure designed to sit flat against the sternum under clothing.

---

## Bill of Materials

### Core Electronics
| Component | Description | Source / Reference |
| :--- | :--- | :--- |
| **Microcontroller** | SparkFun ESP32-S3 Thing Plus (or equivalent ESP32-S3 with JST-PH LiPo, USB-C, and Qwiic) | SparkFun DEV-20982 |
| **Contact Microphone** | PUI Audio VMM-1627L-R MEMS bone-conduction capsule | DigiKey 668-VMM-1627L-R-ND |
| **Preamplifier** | Adafruit MAX4466 Electret Microphone Amplifier (modified) | Adafruit PID 1063 |
| **Haptic Driver** | Adafruit DRV2605L Haptic Motor Controller (STEMMA QT / Qwiic) | Adafruit PID 2305 |
| **Haptic Actuator** | 10 mm ERM Coin Vibration Motor (2.7 mm thick) | Adafruit PID 1201 |
| **Battery** | 1S 3.7V 1100 mAh LiPo Cell (~49.3 x 33.8 x 5.8 mm) with JST-PH 2.0 mm plug | Standard 1S LiPo |
| **Power Switch** | Sub-miniature SPDT slide switch (22.8 mm ear spacing, 11.8 x 5.2 mm body) | Standard slide switch |
| **Qwiic Cable** | 50 mm 4-pin JST-SH cable | SparkFun / Adafruit |
| **Interconnect Wire** | 30 AWG silicone or enameled magnet wire | Generic |

### Fasteners & Hardware
| Item | Specification | Quantity |
| :--- | :--- | :--- |
| **Heat-Set Threaded Inserts** | M2 x 2 mm length x 3.2 mm outer diameter | 4 |
| **Fasteners** | M2 x 4 mm or M2 x 5 mm countersunk socket head cap screws | 4 |
| **Cordage** | Paracord sleeve or 2 mm satin necklace cord | ~1 meter |

### Tools Required
- Temperature-controlled soldering station (chisel and fine-point tips)
- Soldering flux (liquid or gel) and fine solder wire
- Tweezers and helping hands
- Heat gun or lighter (for heat-shrink tubing)
- Wire strippers / flush cutters
- 3D printer (PLA or PETG, 0.4 mm nozzle, 0.16–0.20 mm layer height)
- Soldering iron tip for heat-set insert installation (or standard cone tip)

---

## Hardware Modifications & Wiring

### 1. VMM-1627L-R MEMS Capsule Preparation
Review the [VMM-1627L-R Datasheet](https://puiaudio.com/file/specs-VMM-1627L-R.pdf) for the bottom pad layout before starting:
- **Pad 3:** VDD (Power)
- **Pad 6:** OUT (Analog Signal)
- **Pads 1, 2, 4, 5:** Ground (GND)

> **CRITICAL:** Do not cover or allow glue/solder into the acoustic relief hole on the top face of the capsule. Doing so will destroy the MEMS diaphragm.

1. Secure the microphone capsule inverted on a work surface using Kapton or masking tape, exposing the solder pads.
2. Apply liquid flux to pads 3, 4, and 6.
3. Using dead-bug soldering techniques and fine 30 AWG wire:
   - Solder a red wire to **Pad 3 (VDD)**.
   - Solder a green wire to **Pad 6 (OUT)**.
   - Solder a black wire to **Pad 4 (GND)**.
4. Apply a small drop of epoxy or UV-curable glue around the wire joints for strain relief, avoiding the relief hole.

### 2. MAX4466 Preamplifier Modification
Stock MAX4466 breakout boards incorporate a 2.2k pull-up bias resistor ($R_1$) configured for two-terminal electret capsules. Because the VMM-1627L-R is an active 3-pin MEMS device with an internal buffer, $R_1$ must be removed to avoid DC line saturation:

1. **Remove Electret Capsule:** Desolder the default microphone capsule pins and remove the barrel.
2. **Remove Bias Resistor $R_1$:** Locate $R_1$ connected to the microphone positive input terminal. Apply a blob of solder across both pads to lift the surface-mount resistor off the board. Clean pads with solder wick.
3. **Attach VMM-1627L-R:**
   - Solder the **OUT** wire (green) to the breakout input pad (`+`).
   - Solder the **GND** wire (black) to the breakout ground pad (`-`).
   - Solder the **VDD** wire (red) directly to the `VCC` pin on the MAX4466 board.

### 3. Battery Cable and Switch Integration
1. **Polarity Verification:** Inspect the JST-PH 2.0 mm connector against the SparkFun ESP32-S3 silkscreen markings. If polarity is reversed, lift the connector retention tabs with a precision pick, withdraw the crimp terminals, and swap positions.
2. **Switch Wiring:** Cut the positive (red) lead of the battery harness. Strip and tin both cut ends.
3. Slide heat-shrink tubing over both wires, solder one end to the center common pin of the slide switch, and solder the other end to one of the outer pins. Shrink the tubing to insulate the terminals.

### 4. Haptic Driver and Motor Assembly
1. Connect the Adafruit DRV2605L breakout to the SparkFun ESP32-S3 using a 50 mm Qwiic / STEMMA QT cable.
2. Solder the red wire from the 10 mm ERM coin motor to the `+` motor output pad on the DRV2605L.
3. Solder the blue (or black) wire from the motor to the `-` motor output pad on the DRV2605L.

### 5. Interconnect Pinout

| From: Neck Node (MAX4466) | To: Main Pendant (ESP32-S3 Thing Plus) | Notes |
| :--- | :--- | :--- |
| **VCC** | **3V3** | 3.3V Power Bus |
| **GND** | **GND** | System Ground |
| **OUT** | **GPIO 14** | ADC1 Analog Input |

| From: DRV2605L Driver | To: ESP32-S3 Thing Plus | Notes |
| :--- | :--- | :--- |
| **Qwiic Port** | **Qwiic Port** | Dedicated 3.3V, GND, SDA, SCL bus |

---

## Mechanical Assembly

1. **Insert Installation:** Use a soldering iron set to approximately 220°C to press four M2 x 2 x 3.2 mm brass threaded inserts into the four corner bosses of `Pendant_Base`. Ensure the inserts sit flush with the top lip.
2. **Actuator Placement:** Press the 10 mm coin motor into the circular recess in the floor on the right side of the base. Route motor wires through the wire relief slot.
3. **Battery Placement:** Lay the 1100 mAh LiPo flat in the main center-left battery bay.
4. **Microcontroller Installation:** Set the SparkFun ESP32-S3 Thing Plus onto the internal end ledges. The USB-C port aligns with the cutout on the left wall.
5. **Switch and Mezzanine:**
   - Drop the slide switch into the captive vertical slot along the top wall.
   - Seat the DRV2605L driver on the upper right mezzanine shelves using the printed locator pins.
6. **Neck Harness Routing:** Route the three-wire bundle from the neck sensor through the open U-notch on the upper-left wall.
7. **Lid Closure:** Position `Pendant_Lid` on top of the base and fasten with four M2 countersunk screws.

---

## Firmware Setup & Calibration

### Dependencies
Install the following libraries via the Arduino IDE Library Manager:
- `Adafruit DRV2605 Library`

Ensure the ESP32 board support package (by Espressif) is installed under **Tools → Board → Boards Manager**.

### Configuration Parameters
Adjust these constants at the beginning of the sketch to tune detection:

```cpp
// Target pitch frequency threshold (Hz)
// Values below this frequency trigger a haptic event
#define PITCH_THRESHOLD_HZ 175.0f

// Minimum signal amplitude required before running pitch detection
#define SILENCE_THRESHOLD  200

// DRV2605L ROM Waveform Library Index (e.g., 47 = Strong Buzz 100%, 1 = Strong Click)
#define WAVEFORM           47
