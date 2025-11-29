# ATECC608A Raspberry Pi

## Overview

This project is designed to interface the Microchip ATECC608A Cryptographic Co-processor with a Raspberry Pi. The demo supports TRNG, SHA, and AES.

## Features

- ✅ **Wake-up & Communication**: Initializes and wakes up the ATECC608A over I2C.
- 🔎 **Read Configuration Data**: Reads and displays the device's configuration zone.
- 🔐 **Check Lock Status**: Determines whether the ATECC608A is locked or unlocked.
- 🎲 **Generate Random Numbers**: Utilizes the ATECC608A’s True Random Number Generator.
- 🔢 **Compute SHA-256 Hash**: Computes a SHA-256 hash of a message using the ATECC608A.
- 📜 **Retrieve Serial Number**: Reads and displays the unique serial number of the device.
- 🔐 **AES 128-bit Encryption**: Performs an encryption and decryption operation using ATECC608A.
- 🛠 **I2C Communication**: Implements sending and receiving commands using the Pico I2C interface.

## Hardware Requirements

- Raspberry Pi 3/4/5
- Microchip ATECC608 (A or B variant, I2C interface)
- Pull-up resistors (4.7kΩ) for SDA and SCL
- Jumper wires for I2C connections

Pull-up resistors are not required if you're using a breakout board by Adafruit or Sparkfun.

## Wiring Guide

| Raspberry Pi Pico | ATECC608A |
|-------------------|-----------|
| GP4 (SDA)         | SDA       |
| GP5 (SCL)         | SCL       |
| GND               | GND       |
| 3.3V              | VCC       |

## Software Requirements

Raspberry Pi OS or any variant of Linux for the Pi.

https://www.raspberrypi.com/software/operating-systems/

## Installation & Setup

1. Clone the repository:
```sh
git clone git@github.com:retromanc3r/pi_atecc.git
cd pi_atecc
```

2. Build the project (cmake):
```sh
cmake -S . -B build && cmake --build build
```

3. Run the compiled binary:
```sh
build/pi_atecc
```

Alternatively, you can compile without cmake (gcc):
```sh
gcc -o pi_atecc pi_atecc.c
```

## Usage
1. Run the compiled demo
    ```sh
    ./pi_atecc
    ```

2. Expected Output (Locked) IS configured for AES
    ```
    Waking ATECC608A...
    ⏰ Sending wake command...
    📬 Wake response: 04 11 33 43 
    ✅ ATECC608A is awake!
    🆔 Serial Number: 0123BFBDEA185823EE
    🎲 Random number in range 0-10000000: 3246637
    🎰 Random Value: 7F 1C AF CB 23 FF 9B 37 D3 AA 32 30 7F 40 15 76 
    🔒 SHA-256: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
    🔎 Checking Slot 3 Configuration...
    🔎 Slot 3 Config Data: 07 EE 61 5D
    🔎 Reading Configuration Data...
    01 23 BF BD 00 00 60 03 EA 18 58 23 EE 61 5D 00
    C0 00 00 00 87 20 C7 77 E7 77 07 07 C7 77 E7 77
    07 07 87 07 07 07 07 07 C0 07 C0 0F 9D BD 8D 4D
    07 07 00 47 00 3F 00 00 00 1F 00 1E 00 FF 00 00
    00 1F 00 1E 00 00 00 00 00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00 FF FF 00 00 00 00 00 00
    73 00 38 00 38 00 18 00 7C 00 7C 00 1C 00 7C 00
    3C 00 30 00 3C 00 3C 00 B8 0D 7C 00 30 00 3C 00
    🔍 Checking ATECC608A Lock Status...
    🔐 Raw Lock Status Response: 07 00 00 00 00 03 AD 
    🔒 Config Lock Status: 00
    🔒 Data Lock Status: 00
    🔒 Chip is **FULLY LOCKED** (Config & Data).
    🔐 Performing AES 128-bit Encryption/Decryption using Slot 3...
    🔹 Plaintext: 48 65 6C 6C 6F 2C 20 41 45 53 21 00 00 00 00 00 
    🔹 Ciphertext: 07 96 94 A6 9E A6 57 E8 43 D9 4A 60 34 D3 38 15 
    🔹 Decrypted: 48 65 6C 6C 6F 2C 20 41 45 53 21 00 00 00 00 00 
    ✅ AES Decryption Successful! Plaintext Matches!
    🌙 Putting ATECC608A to sleep...
    🎉 ATECC608A Test Complete!
    ```
3. Expected Output (Locked) NOT configured for AES
    ```
    Waking ATECC608A...
    ⏰ Sending wake command...
    📬 Wake response: 04 11 33 43 
    ✅ ATECC608A is awake!
    🆔 Serial Number: 0123EAA25AB7C470EE
    🎲 Random number in range 0-10000000: 832769
    🎰 Random Value: 92 94 B4 81 08 1E D9 DF 28 D3 A4 E4 A9 C8 18 5A 
    🔒 SHA-256: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
    🔎 Checking Slot 3 Configuration...
    🔎 Slot 3 Config Data: 07 EE 61 4D
    🔎 Reading Configuration Data...
    01 23 EA A2 00 00 60 03 5A B7 C4 70 EE 61 4D 00
    C0 00 00 00 83 20 87 20 8F 20 C4 8F 8F 8F 8F 8F
    9F 8F AF 8F 00 00 00 00 00 00 00 00 00 00 00 00
    00 00 AF 8F 00 07 00 00 00 01 00 00 FF FF FF FF
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00 FF FF 00 00 00 00 00 00
    33 00 33 00 33 00 1C 00 1C 00 1C 00 1C 00 1C 00
    3C 00 3C 00 3C 00 3C 00 3C 00 3C 00 3C 00 1C 00
    🔍 Checking ATECC608A Lock Status...
    🔐 Raw Lock Status Response: 07 00 00 00 00 03 AD 
    🔒 Config Lock Status: 00
    🔒 Data Lock Status: 00
    🔒 Chip is **FULLY LOCKED** (Config & Data).
    🔐 Performing AES 128-bit Encryption/Decryption using Slot 3...
    🔹 Plaintext: 48 65 6C 6C 6F 2C 20 41 45 53 21 00 00 00 00 00 
    receive_aes_response: device status 0x0F
    aes_encrypt: AES encrypt response failed
    ❌ AES 128-bit encryption failed!
    ❓ Is the slot configured for AES?
    ```
4. Expected Output (Unlocked) NOT configured for AES:
    ```
    Waking ATECC608A...
    ⏰ Sending wake command...
    📬 Wake response: 04 11 33 43 
    ✅ ATECC608A is awake!
    🆔 Serial Number: 0123703AA642748FEE
    🎲 Random number in range 0-10000000: 7900326
    🎰 Random Value: FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
    🔒 SHA-256: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
    🔎 Checking Slot 3 Configuration...
    🔎 Slot 3 Config Data: 07 EE 61 61 
    🔎 Reading Configuration Data...
    01 23 70 3A 00 00 60 03 A6 42 74 8F EE 61 61 00 
    C0 00 00 00 83 20 87 20 8F 20 C4 8F 8F 8F 8F 8F 
    9F 8F AF 8F 00 00 00 00 00 00 00 00 00 00 00 00 
    00 00 AF 8F FF FF FF FF 00 00 00 00 FF FF FF FF 
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
    00 00 00 00 00 00 55 55 FF FF 00 00 00 00 00 00 
    33 00 33 00 33 00 1C 00 1C 00 1C 00 1C 00 1C 00 
    3C 00 3C 00 3C 00 3C 00 3C 00 3C 00 3C 00 1C 00 
    🔍 Checking ATECC608A Lock Status...
    🔐 Raw Lock Status Response: 07 00 00 55 55
    🔒 Config Lock Status: 55
    🔒 Data Lock Status: 55
    🔓 Chip is **UNLOCKED**.
    🔹 Plaintext: 48 65 6C 6C 6F 2C 20 41 45 53 21 00 00 00 00 00 
    🔐 Performing AES 128-bit Encryption/Decryption using Slot 3...
    🔹 Plaintext: 48 65 6C 6C 6F 2C 20 41 45 53 21 00 00 00 00 00 
    receive_aes_response: device status 0x03
    aes_encrypt: AES encrypt response failed
    ❌ AES 128-bit encryption failed!
    ❓ Is the slot configured for AES?
    ```


## Bill of Materials (BOM)

| **Part**                     | **Description**                                      | **Quantity** | **Notes** | **Order Link** |
|------------------------------|--------------------------------------------------|------------|----------|--------------|
| **Raspberry Pi**  | Raspberry Pi 3, 4, or 5                 | 1          | Any RPi model should do. | [Adafruit](https://www.adafruit.com/product/5813) / [Mouser](https://www.mouser.com/c/embedded-solutions/computing/?form%20factor=Raspberry%20Pi%205) |
| **ATECC608 Breakout Board**    | Adafruit ATECC608 STEMMA QT / Qwiic                | 1          | Any I2C-based ATECC608 board will work | [Adafruit](https://www.adafruit.com/product/4314) / [DigiKey](https://www.digikey.com/en/products/detail/adafruit-industries-llc/4314/10419053) |
| **4.7kΩ Resistors**           | Pull-up resistors for I2C lines            | 2          | Optional if using STEMMA QT or built-in pull-ups | [Amazon](https://www.amazon.com/Elegoo-Values-Resistor-Assortment-Compliant/dp/B072BL2VX1) |
| **Breadboard**                | Prototyping board                                 | 1          | Any standard size | [Adafruit](https://www.adafruit.com/product/239) / [DigiKey](https://www.digikey.com/en/products/detail/global-specialties/GS-830/5231309) |
| **Jumper Wires**              | Male-to-male / Male-to-female wires               | 6+         | For I2C and power connections | [Adafruit](https://www.adafruit.com/product/1957) / [DigiKey](https://www.digikey.com/en/products/filter/jumper-wire/640) |
| **USB Cable**                 | USB to Micro-USB / USB-C                          | 1          | For flashing firmware | [Adafruit](https://www.adafruit.com/product/592) |
| **Raspberry Pi Debug Probe**  | Debugging interface for OpenOCD (optional)        | 1          | Used for debugging/flashing firmware | [Adafruit](https://www.adafruit.com/product/5699) / [Mouser](https://www.mouser.com/ProductDetail/Adafruit/5699) |

## Hardware Setup
Below is an example wiring setup for the ATECC608 with the Raspberry Pi Pico:

![ATECC608 Pico Wiring](images/pi_atecc_wiring.jpg)

[Sparkfun](https://www.sparkfun.com) has a really sharp looking ATECC608A Cryptographic Co-processor [breakout](https://www.sparkfun.com/sparkfun-cryptographic-co-processor-breakout-atecc608a-qwiic.html) as well. Any of these should work.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Author

@retromanc3r

## Acknowledgments

Special thanks to the Pico SDK community and Microchip for their excellent documentation and CryptoAuthLib for the ATECC608. Also, credits to GitHub Copilot for assisting in code generation, but all debugging, I2C bus analysis, and code optimization were performed manually.