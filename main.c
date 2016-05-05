/* 
 * File:   main.C
 * Author: CraigVella
 *
 * Created on September 10, 2013, 10:59 PM
 */

#include "main.h"

unsigned int _displayNum = 0;

static const unsigned long int powerArray[4] = {1,10,100,1000};
static const float mmMPHConversion = 0.0022369362920544025f;

unsigned short _tireSize = 0; // In millimeter * 10
unsigned char bufferedRawKeyState[4] = {0};
static unsigned int keyWait[2] = {0};
unsigned char lastKeyState[4] = {0};
unsigned int _overflowCounter = 0;
unsigned char _LEDOn = 0;
unsigned char LEDFlash = 0;
unsigned int _magCount = 0;
unsigned int _currentMPH = 0;
unsigned char _ProgramState = PRGSTATE_MPH;
unsigned char _LastProgramState = PRGSTATE_MPH;
unsigned char _ProgramStateRequestChange = PRGSTATE_NOREQ;

void interrupt procInt() {
    static unsigned int toCount = 0;

    if (IOCAF5 && IOCIF) {
        IOCAF = 0;
        ++_magCount;
    }

    if (TMR2IE && TMR2IF) {
        // timer 0 interrupt
        TMR2IF = 0; // Reset the flag
        // Overflowed @ 256 ticks
        toCount += 256;

        if (toCount >= 6250) {
            // It has been one second or has passed our one second count
            toCount -= 6250;
            // Do Seccond Function Here
            oneSeccond();
        }
    }
}

void oneSeccond() {
    // This function gets called Once every One Seccond
    if (LEDFlash) { // LED Flashing
        if (_LEDOn)
            _LEDOn = 0;
        else
            _LEDOn = 1;
    }
    if (_tireSize > 0) {
        // How many Times did we go around?
        unsigned long int uliDistance = _tireSize * _magCount;
        // Distance in milli/sec
        _currentMPH = ((uliDistance * mmMPHConversion) * 10);
        _magCount = 0; // reset mag count
    }
}

/* Program State Template
if (_LastProgramState != _ProgramState) {
    // Initialization of This new Program State
}
_LastProgramState = _ProgramState;
*/

void prgScreenMPH() {
    if (_LastProgramState != _ProgramState) {
        // Initialization of This new Program State
        LEDFlash = 0;
        _LEDOn = 0;
    }

    // Key 3 Handling
    if (bufferedRawKeyState[3] != lastKeyState[3]) {
        lastKeyState[3] = bufferedRawKeyState[3];
        if (!bufferedRawKeyState[3]) {
            _ProgramStateRequestChange = PRGSTATE_TSS;
        }
    }

    _displayNum = _currentMPH;

    _LastProgramState = _ProgramState;
}

void prgScreenTSS() {
    static unsigned short changeTireSize = 0;

    if (_LastProgramState != _ProgramState) {
        // Initialization of This new Program State
        LEDFlash = 1;
        changeTireSize = _tireSize;
    }

    // Key 0 Handling
    if (bufferedRawKeyState[0] != lastKeyState[0]) {
        lastKeyState[0] = bufferedRawKeyState[0];
        keyWait[0] = 0;
        keyWait[1] = 0;
        if (!bufferedRawKeyState[0]) {
            if (changeTireSize < 999)
                ++changeTireSize;
        }
    } else if (!bufferedRawKeyState[0]) {
        if (keyWait[0] >= KEY_INITIALWAIT) {
            // Initial Key Wait
            if (keyWait[1] >= KEY_CONSEQWAIT) {
                if (changeTireSize < 999)
                    ++changeTireSize;
                keyWait[1] = 0;
            } else {
                ++keyWait[1];
            }
        } else {
            ++keyWait[0];
        }
    }

    // Key 1 Handling
    if (bufferedRawKeyState[1] != lastKeyState[1]) {
        lastKeyState[1] = bufferedRawKeyState[1];
        keyWait[0] = 0;
        keyWait[1] = 0;
        if (!bufferedRawKeyState[1]) {
            if (changeTireSize > 0)
                --changeTireSize;
        }
    } else if (!bufferedRawKeyState[1]) {
        if (keyWait[0] >= KEY_INITIALWAIT) {
            // Initial Key Wait
            if (keyWait[1] >= KEY_CONSEQWAIT) {
                if (changeTireSize > 0)
                    --changeTireSize;
                keyWait[1] = 0;
            } else {
                ++keyWait[1];
            }
        } else {
            ++keyWait[0];
        }
    }

    // Key 2 Handling
    if (bufferedRawKeyState[2] != lastKeyState[2]) {
        lastKeyState[2] = bufferedRawKeyState[2];
        if (!bufferedRawKeyState[2]) {
            // Cancel Changes
            _ProgramStateRequestChange = PRGSTATE_MPH;
        }
    }

    // Key 3 Handling
    if (bufferedRawKeyState[3] != lastKeyState[3]) {
        lastKeyState[3] = bufferedRawKeyState[3];
        if (!bufferedRawKeyState[3]) {
            // Cancel Changes
            if (_tireSize != changeTireSize)
                saveTireSize(changeTireSize);
            _ProgramStateRequestChange = PRGSTATE_MPH;
        }
    }
    
    _displayNum = (changeTireSize * 10);

    _LastProgramState = _ProgramState;
}

void main() {
    unsigned char waitCount = 0;

    initialize();

    if (_tireSize == 0) _ProgramState = PRGSTATE_TSS;

    while (1) {
        if (_ProgramStateRequestChange != PRGSTATE_NOREQ) { // Check bounds for Program state change
            _ProgramState = _ProgramStateRequestChange;

            if (_tireSize == 0)
                _ProgramState = PRGSTATE_TSS;

            _ProgramStateRequestChange = PRGSTATE_NOREQ;
        }
        
        keyDebounce(); // Debounce the Keys

        switch (_ProgramState) {
            case PRGSTATE_TSS:
                prgScreenTSS();
                break;
            case PRGSTATE_MPH:
                prgScreenMPH();
            default:
                break;
        }

        if (++waitCount >= 30) { // Wait 30 Loops then Move to next display
            updateDisplay();
            waitCount = 0;
        }
    }

}

void keyDebounce() {
    static unsigned char keyCount[4] = {0};
    static unsigned char keyPoll = 0;
    static unsigned char lastKeyState[4] = {0};
    unsigned char thisKeyState = (PORTB >> (keyPoll + 4)) & 1;

    if(thisKeyState == lastKeyState[keyPoll]) {
        if (++keyCount[keyPoll] >= 30) {
            bufferedRawKeyState[keyPoll] = thisKeyState;
            keyCount[keyPoll] = 0; // No need to roll that over if holding it down
        }
    } else {
        keyCount[keyPoll] = 0;
        lastKeyState[keyPoll] = thisKeyState;
    }

    if (++keyPoll >= 4)
        keyPoll = 0;
}

void updateDisplay() {
    unsigned int bufferedValue = _displayNum;
    static unsigned char currentDisplay = 0;

    PORTC = (_LEDOn << 6) + (((bufferedValue / powerArray[currentDisplay]) % 10) << 2) + currentDisplay;

    if (++currentDisplay >= 4)
        currentDisplay = 0;
}

void initialize() {
    OSCCON = 0b01111000; // 16 MHZ and Select From FOSC Config
    FVRCON = 0b11001000; // Comparator Fixed Reference Voltage at 2.048V

    // Go All Digital
    ANSELA = 0; // Set digtial all on A then Analog on A-1
    ANSELAbits.ANSA1 = 1; // Analog Input on PORTA-1
    ANSELB = 0; // Digital In on PortB
    ANSELC = 0; // Digital Out on PortC

    TRISA = 0; // TRISA all Out
    TRISAbits.TRISA1 = 1; // Input for Low Batt
    TRISAbits.TRISA5 = 1; // Input for Magnetic Sensor
    TRISB = 0xFF; // All Input
    TRISC = 0;

    PORTA = 0;
    PORTB = 0;
    PORTC = 0;

    // Turn on Comparater for Low Batt
    CM1CON0 = 0b11100000;
    CM1CON1 = 0b00100000;

    // Start up Timer 2
    T2CON = 0b01001111; // Prescaled to 64 (62500) then Postscaled to 10 (6250)

    PIE1bits.TMR2IE = 1; // Enable TMR2 Interrupt
    INTCONbits.PEIE = 1; // Enable Peripheral Interrupt
    INTCONbits.IOCIE = 1; // Enable Interrupt on Change

    IOCAP5 = 1; // Enable IOC on A5 on Positive Edge

    WPUA = 0; // Turn Off WP on Port A
    WPUB = 0xFF; // Turn On WP on Port B (Switches)

    OPTION_REGbits.nWPUEN = 0; // Enable Weak Pull UPs

    // Load _tireSize from ProgramMemory @ 1FE0; 1 Word;
    PMCON1bits.CFGS = 0;
    PMADRH = 0x1F;
    PMADRL = 0xE0;
    PMCON1bits.RD = 1;
    _nop();
    _nop();
    _tireSize = PMDAT;
    
    if (_tireSize > 999)
        _tireSize = 0;

    GIE = 1; // Enable all Interrupts
}

void saveTireSize(unsigned short newTireSize) {
    if (newTireSize > 999)
        return;
    GIE = 0; // Shut off Interrupts

    // First Erase the Memory
    PMCON1bits.WREN = 1; // Write Enable
    PMCON1bits.CFGS = 0;
    PMADRH = 0x1F;
    PMADRL = 0xE0;
    PMCON1bits.FREE = 1; // Erase

    PMCON2 = 0x55; // Unlock Seq
    PMCON2 = 0xAA;
    PMCON1bits.WR = 1;
    _nop();
    _nop();

    PMADR = 0x1FE0; // 32 Bytes till end, Start 0x1FE0 till 0x1FFF

    PMCON1bits.CFGS = 0;
    PMCON1bits.FREE = 0;
    PMCON1bits.WREN = 1;
    PMCON1bits.LWLO = 1; // Writing to Latches

    PMDAT = newTireSize;
    // Here we would loop and up the Lower bits till we hit 32 then do a write if we had more data
    // If you need to loop you need to Re Unlock Seq for every latch except last one
    
    PMCON1bits.LWLO = 0; // Done writing Latches

    PMCON2 = 0x55; // Unlock Seq
    PMCON2 = 0xAA;
    PMCON1bits.WR = 1;
    _nop();
    _nop();
    
    PMCON1bits.WREN = 0; // Back to normal
    _tireSize = newTireSize;
    GIE = 1; // Turn it back up
}