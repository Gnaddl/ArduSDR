/*
 * ArduSDR - Arduino controlled Software Defined Radio
 *
 * supports Elektor SDR Shield (https://www.elektormagazine.com/magazine/elektor-201807/41737)
 * emulates Yaesu FT-991 CAT protocol over serial line
 *
 * https://github.com/Gnaddl/ArduSDR
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Encoder.h>
#include <si5351.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Encoder rotaryEncoder(2, 3);
Si5351 si5351;

unsigned long frequencyVfoA = 14074000UL;   // VFO-A frequency in Hz; start with 14.074 MHz
unsigned long frequencyVfoB = 7074000UL;    // VFO-B frequency in Hz; start with 7.074 MHz
unsigned long &frequency = frequencyVfoA;
int catAutoInformation = 1;                 // 0 = off, 1 = on
int catOperatingMode = 2;                   // 1 = LSB, 2 = USB, ...
int catBandWidth = 0;
int catTx = 0;
int catPowerSwitch = 1;                     // 0 = power off, 1 = power on
int catTxSet = 0;
int catKeySpeed = 10;
int catNarrow = 0;


/*
 * setfreq - set the RX frequency in the SI5351 and display it on the LCD
 */
void setfreq(unsigned long frequency)
{
    uint64_t freq = frequency * 100ULL;

    char buffer[20];
    sprintf(buffer, "%5u.%03u kHz", (unsigned int)(frequency / 1000UL), (unsigned int)(frequency % 1000UL) / 10);
    lcd.setCursor(0, 1);
    lcd.print(buffer);

    lcd.setCursor(6, 1);
    lcd.cursor();

    si5351.set_freq(freq, SI5351_CLK0);
}


/*
 * General structure of the CAT support functions:
 *
 * Calling parameters:
 * pBuffer:  points to a buffer with a '\0'-terminated string containing the
 *           parameter field of the received CAT command.
 * rxlength: >0:  number of characters in the parameter field in pBuffer
 *           =0:  no parameter was provided
 *
 * Return data:
 * pBuffer:  If an answer to a CAT read command is required, the content of
 *           its parameter field must be written into the same buffer. The
 *           command and the terminator are then added later by the caller of
 *           the function.
 * Return value:
 *           >0:  the number of characters in pBuffer for the parameter field
 *                of the CAT command answer
 *           =0:  no answer shall be sent back
 */

int doAutoInformation(char *pBuffer, int rxlength)
{
    if (rxlength > 0)
    {
        // Set command
        catAutoInformation = atoi(pBuffer);
        return 0;
    }
    else
    {
        // Read command
        return sprintf(pBuffer, "%d", catAutoInformation);
    }
}


int doMenu(char *pBuffer, int rxlength)
{
    static char menuBuffer[32] = "";

    if (rxlength > 3)
    {
        // Set command
        strcpy(menuBuffer, pBuffer + 3);
        return 0;
    }
    else if (rxlength == 3)
    {
        // Read command
        return sprintf(pBuffer + 3, "%s", menuBuffer) + 3;
    }
    else
    {
        // Command is too short, ignore.
        return 0;
    }
}


int doFrequencyVfoA(char *pBuffer, int rxlength)
{
      if (rxlength > 0)
      {
          // Set command
          frequencyVfoA = atol(pBuffer);
          setfreq(frequency);
          return 0;
      }
      else
      {
          // Read command
          return sprintf(pBuffer, "%09lu", frequencyVfoA);
      }
}


int doFrequencyVfoB(char *pBuffer, int rxlength)
{
      if (rxlength > 0)
      {
          // Set command
          frequencyVfoB = atol(pBuffer);
          setfreq(frequency);
          return 0;
      }
      else
      {
          // Read command
          return sprintf(pBuffer, "%09lu", frequencyVfoB);
      }
}


int doFunctionTx(char *pBuffer, int rxlength)
{
      if (rxlength > 0)
      {
          // Set command
          catTx = atoi(pBuffer);
          return 0;
      }
      else
      {
          // Read command
          return sprintf(pBuffer, "%d", catTx);
      }
}


int doIdentification(char *pBuffer, int rxlength)
{
    // Read command only
    strcpy(pBuffer, "0570");
    return 4;
}


int doInformation(char *pBuffer, int rxlength)
{
    // Read command only
    return sprintf(pBuffer, "001%09lu+000000%X00000", frequencyVfoA, catOperatingMode);
}


int doKeySpeed(char *pBuffer, int rxlength)
{
    if (rxlength > 0)
    {
        // Set command
        catKeySpeed = atoi(pBuffer);
        return 0;
    }
    else
    {
        // Read command
        return sprintf(pBuffer, "%03d", catKeySpeed);
    }
}


int doOperatingMode(char *pBuffer, int rxlength)
{
    if (rxlength > 1)
    {
        // Set command
        catOperatingMode = atoi(pBuffer + 1);
        return 0;
    }
    else
    {
        // Read command
        return sprintf(pBuffer, "0%d", catOperatingMode);
    }
}


int doNarrow(char *pBuffer, int rxlength)
{
    if (rxlength > 1)
    {
        // Set command
        catNarrow = atoi(pBuffer + 1);
        return 0;
    }
    else
    {
        // Read command
        return sprintf(pBuffer, "0%d", catNarrow);
    }
}


int doPowerSwitch(char *pBuffer, int rxlength)
{
    if (rxlength > 0)
    {
        // Set command
        catPowerSwitch = atoi(pBuffer);
        return 0;
    }
    else
    {
        // Read command
        return sprintf(pBuffer, "%d", catPowerSwitch);
    }
}


int doBandwidth(char *pBuffer, int rxlength)
{
    if (rxlength > 1)
    {
        // Set command
        catBandWidth = atoi(pBuffer + 1);
        return 0;
    }
    else
    {
        // Read command
        return sprintf(pBuffer, "0%02d", catBandWidth);
    }
}


int doTxSet(char *pBuffer, int rxlength)
{
    if (rxlength > 0)
    {
        // Set command
        catTxSet = atoi(pBuffer);
        return 0;
    }
    else
    {
        // Read command
        return sprintf(pBuffer, "%d", catTxSet);
    }
}


static const struct catCmdItem
{
    int cmd;
    int (*fn)(char *, int);
} catCmdList[] =
{
    { 0x4149, doAutoInformation },    // AI: Auto Information
    { 0x4558, doMenu },               // EX: Menu
    { 0x4641, doFrequencyVfoA },      // FA: Frequency VFO-A
    { 0x4642, doFrequencyVfoB },      // FB: Frequency VFO-B
    { 0x4654, doFunctionTx },         // FT: Function TX
    { 0x4944, doIdentification },     // ID: Identification
    { 0x4946, doInformation },        // IF: Information
    { 0x4B53, doKeySpeed },           // KS: Key Speed
    { 0x4D44, doOperatingMode },      // MD: Operating Mode
    { 0x4E41, doNarrow },             // NA: Narrow
    { 0x5053, doPowerSwitch },        // PS: Power Switch
    { 0x5348, doBandwidth },          // SH: Bandwidth
    { 0x5458, doTxSet },              // TX: TX Set
    { 0x0000, NULL }                  // End marker, don't delete!
};


/*
 * handleSerial - read characters from the serial line and interpret them
 *                according to the Yaesu FT-991 CAT protocol.
 */
void handleSerial(void)
{
    static char rxbuffer[64];
    static unsigned int rxlength = 0;

    while (Serial.available() > 0)
    {
        int c = Serial.read();

        if (!isprint(c) || isspace(c))
        {
            // Ignore non-printable and whitespace characters.
            continue;
        }

        if (c != ';')
        {
            // No termination character received, save the character in the
            // receive buffer.
            if (rxlength < sizeof(rxbuffer) - 2)
            {
                rxbuffer[rxlength++] = toupper(c);
            }
            continue;
        }

        // Termination character was received. Check if was a potential command.
        if (rxlength < 2)
        {
            // Not enough characters, clear the receive buffer.
            rxlength = 0;
            continue;
        }

        // Add a string termination character to the buffer.
        rxbuffer[rxlength] = '\0';

#ifdef DEBUG
        lcd.setCursor(0, 0);
        lcd.print("                ");
        lcd.setCursor(0, 0);
        lcd.print(rxbuffer);
        lcd.print(";");
        //delay(500);
#endif

        // Build the CAT command code from the first two characters in the
        // receive buffer.
        int cmd = (rxbuffer[0] << 8) | rxbuffer[1];
        int rc = 0;

        // Search the CAT command in the command list.
        const catCmdItem *pCatCmd;
        for (pCatCmd = catCmdList;
             (pCatCmd->cmd != cmd) && (pCatCmd->cmd != 0);
             ++pCatCmd);

        if (pCatCmd->cmd == cmd)
        {
            // CAT command found, call the corresponding CAT support function.
            rc = pCatCmd->fn(rxbuffer + 2, rxlength - 2);
        }
        else
        {
            // CAT command not found, return an error message.
            strcpy(rxbuffer + 2, "???");
            rc = 3;
        }

        if (rc > 0)
        {
            // The command characters for the answer are already in the buffer.
            // Therefore, nothing needs to be done here.

            // Add the terminator (trailing semicolon).
            strcat(rxbuffer, ";");

            // Send the answer to the CAT command.
            Serial.print(rxbuffer);

#ifdef DEBUG
            lcd.setCursor(0, 1);
            lcd.print("                ");
            lcd.setCursor(0, 1);
            lcd.print(rxbuffer);
            //delay(500);
#endif
        }

        rxlength = 0;           // Received CAT command completed.
    }
}


/*
 * handleRotaryEncoder - get rotation from encoder and handle it
 */
void handleRotaryEncoder(void)
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
 * loop - periodically called by the main loop, handle serial communication
 *        and encoder rotation.
 */
void loop(void)
{
    handleSerial();
    handleRotaryEncoder();
}
