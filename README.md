# Arduino Project ArduSDR

This is my Arduino sketch for a Software Defined Radio, using an **Elektor SDR Shield 2.0** connected to an **Arduino Uno**. The SDR shield was released in Elektor Magazine 7/2018 with some basic firmware. It can be controlled by a PC via the Arduino's USB interface, but only few SDR programs support the original command set, e.g. [G8JCFSDR](https://www.g8jcf.uk/). Meanwhile the firmware had some improvements, but it is still very basic and has its limitations. Therefore, I wrote my own software for fun and added a few useful functions:

+ A subset of the well documented **Yaesu FT-991 CAT serial protocol** is used instead of the proprietary serial protocol from Elektor. This allows you to control the SDR hardware by more SDR programs on Windows PCs, on Linux PCs or on Macs.
+ A standard **2 lines by 16 characters LCD** with an I²C interface is used for displaying useful information about the status of the SDR. The display is connected to the Arduino's I²C pins SCL and SDA and its I²C address is 0x27.
+ A **rotary encoder** can be used to change the RX frequency as in commercial amateur radio transceivers.
+ *TODO*: A **4 x 4 keypad** can be used to enter the RX frequency directly. The keypad needs only one analog input pin on the Arduino.

## Serial Commands

The ArduSDR supports the following subset of the most commonly used Yaesu FT-991 CAT protocol commands. See [2] for a more detailed description.

| Command | Description      |
| ------- | ---------------- |
| AI      | Auto Information |
| FA      | Frequency VFO-A  |
| FT      | Function TX      |
| ID      | Identification   |
| IF      | Information      |
| MD      | Operating Mode   |
| PS      | Power Switch     |
| SH      | Width            |
| TX      | TX Set           |

## Used Libraries

+ [Etherkit Si5351](https://github.com/etherkit/Si5351Arduino) version 2.1.4 by Jason Milldrum
+ [LiquidCrystal I2C](https://github.com/johnrickman/LiquidCrystal_I2C) version 1.1.2 by Marco Schwartz
+ [Encoder](https://www.pjrc.com/teensy/td_libs_Encoder.html) version 1.4.2 by Paul Stoffregen
+ *TODO*: [OnewireKeypad](https://github.com/AndrewMascolo/OnewireKeypad) version 0.0.0 by Andrew Mascolo

## Ideas for Future Development

+ Integrated WSPR encoding and modulation
+ Morse code encoder and decoder
+ RTTY encoder and decoder

## Links

[1] Elektor SDR shield: https://www.elektormagazine.com/magazine/elektor-201807/41737

[2] Yaesu FT-991 CAT Operation Reference Manual: https://www.yaesu.com/downloadFile.cfm?FileID=10604&FileCatID=158&FileName=FT%2D991%5FCAT%5FOM%5FENG%5F1612%2DD0.pdf&FileContentType=application%2Fpdf

