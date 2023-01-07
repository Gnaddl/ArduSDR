/*
 * ArduSDR - Arduino controlled Software Defined Radio
 *
 * supports Elektor SDR Shield (https://www.elektormagazine.com/magazine/elektor-201807/41737)
 * emulates Yaesu FT-991 CAT protocol over serial line
 *
 * https://github.com/Gnaddl/ArduSDR
 */

#include <LiquidCrystal_I2C.h>
#include <Encoder.h>
#include <si5351.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Encoder rotaryEncoder(2, 3);
Si5351 si5351;

unsigned long frequency = 3573000UL;    // RX frequency in Hz; start with 3573 kHz
int catOperatingMode = 1;               // 1 = LSB
int catBandWidth = 0;
int catTx = 0;
int catPowerSwitch = 1;
int catTxSet = 0;


/*
 * setfreq - set the RX frequency in the SI5351 and display it on the LCD
 */
void setfreq (unsigned long frequency)
{
    uint64_t freq = frequency * 100ULL;

    char buffer[20];
    sprintf(buffer, "%5u.%02u kHz", (unsigned int)(frequency / 1000UL), (unsigned int)(frequency % 1000UL) / 10);
    lcd.setCursor(0, 1);
    lcd.print(buffer);

    lcd.setCursor(6, 1);
    lcd.cursor();

    si5351.set_freq(freq, SI5351_CLK0);
}


/*
 * pollSerial - read characters from the serial line and interpret them according
 * to the Yaesu FT-991 CAT protocol.
 */
void pollSerial(void)
{
    static char rxbuffer[32];
    static char txbuffer[32];
    static unsigned int rxlength = 0;

    while (Serial.available() > 0)
    {
        int c = Serial.read();

        if (!isprint(c) || isspace(c))
        {
            return;
        }

        if (c != ';')
        {
            // No termination character received, save the character in the receive buffer.
            if (rxlength < sizeof(rxbuffer) - 2)
            {
                rxbuffer[rxlength++] = toupper(c);
            }
            return;
        }

        // Termination character received, command is complete. Try to interpret it.
        if (rxlength < 2)
        {
            // Command is too short, ignore it and clear the receive buffer.
            rxlength = 0;
            return;
        }

        rxbuffer[rxlength] = '\0';
        *txbuffer = '\0';

#ifdef DEBUG
        lcd.setCursor(0, 0);
        lcd.print("                ");
        lcd.setCursor(0, 0);
        lcd.print(rxbuffer);
        lcd.print(";");
        //delay(500);
#endif

        // Get the command from the first two characters in the receive buffers.
        int cmd = (rxbuffer[0] << 8) | rxbuffer[1];
        switch (cmd)
        {
            case 0x4149:        // AI: Auto Information
                if (rxlength == 2)
                {
                    strcpy(txbuffer, "AI1;");
                }
                break;

            case 0x4641:        // FA: Frequency VFO-A
                if (rxlength == 2)
                {
                    // Read
                    sprintf(txbuffer, "FA%09lu;", frequency);
                }
                else
                {
                    // Write
                    frequency = atol(&rxbuffer[2]);
                    setfreq(frequency);
                }
                break;

            case 0x4654:        // FT: Function TX
                if (rxlength == 2)
                {
                    // Read
                    sprintf(txbuffer, "FT%d;", catTx);
                }
                else
                {
                    // Write
                    catTx = atoi(&rxbuffer[2]) - 2;
                }
                break;

            case 0x4944:        // ID: Identification
                if (rxlength == 2)
                {
                    strcpy(txbuffer, "ID0570;");
                }
                break;

            case 0x4946:        // IF: Information
                if (rxlength == 2)
                {
                    sprintf(txbuffer, "IF001%09lu+000000%X00000;", frequency, catOperatingMode);
                }
                break;

            case 0x4D44:        // MD: Operating Mode
                if (rxlength == 3)
                {
                    // Read Mode
                    sprintf(txbuffer, "MD0%d;", catOperatingMode);
                }
                else
                {
                    // Set Mode
                    catOperatingMode = atoi(&rxbuffer[3]);
                }
                break;

            case 0x5053:        // PS: Power Switch
                if (rxlength == 2)
                {
                    // Read
                    sprintf(txbuffer, "PS%d;", catPowerSwitch);
                }
                else
                {
                    // Write
                    catPowerSwitch = atoi(&rxbuffer[2]);
                }
                break;

            case 0x5348:        // SH: Width
                if (rxlength == 3)
                {
                    // Read Width
                    sprintf(txbuffer, "SH0%02d;", catBandWidth);
                }
                else
                {
                    // Set Width
                    catBandWidth = atoi(&rxbuffer[3]);
                }
                break;

            case 0x5458:        // TX: TX Set
                if (rxlength == 2)
                {
                    // Read
                    sprintf(txbuffer, "TX%d;", catTxSet);
                }
                else
                {
                    // Write
                    catTxSet = atoi(&rxbuffer[2]);
                }
                break;

            default:            // unknown command
                strcpy(txbuffer, "????????");
                delay(1000);
                break;
        }

        if (strlen(txbuffer) > 0)
        {
            Serial.print(txbuffer);

#ifdef DEBUG
            lcd.setCursor(0, 1);
            lcd.print("                ");
            lcd.setCursor(0, 1);
            lcd.print(txbuffer);
            //delay(500);
#endif
        }

        rxlength = 0;           // Received command completed.
    }
}


/*
 * pollRotaryEncoder - get rotation from encoder and handle it
 */
void pollRotaryEncoder(void)
{
    long diffPosition = rotaryEncoder.readAndReset();
    if (diffPosition != 0)
    {
        frequency += diffPosition * 100L;
        setfreq(frequency);
    }
}


/*
 * setup - hardware setup
 */
void setup(void)
{
    Serial.begin(9600);

    lcd.init();
    lcd.backlight();
    lcd.print("ArduSDR");

    // Initialise the clock generator
    bool i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if(!i2c_found)
    {
        Serial.println("SI5351 not found on I2C bus!");
        lcd.setCursor(0, 1);
        lcd.print("SI5351 ERROR");
        while(1);
    }

    //si5351.enableOutputs(true);
    //si5351.setupPLL(SI5351_PLL_A, 36, 0, 1000);  //900 MHz

    setfreq(frequency);

    // Query a status update and wait a bit to let the Si5351 populate the
    // status flags correctly.
    si5351.update_status();
    delay(500);
}


/*
 * loop - periodically called by the main loop, handles serial communication and encoder rotation.
 */
void loop(void)
{
    pollSerial();
    pollRotaryEncoder();
}
