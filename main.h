/* 
 * File:   main.h
 * Author: CraigVella
 *
 * Created on September 10, 2013, 10:59 PM
 */

#include "ConfigBits.h"


#define KEY_INITIALWAIT 8000
#define KEY_CONSEQWAIT  500

void initialize();

void updateDisplay();
void keyDebounce();
//void handleUserInput();
void oneSeccond();

void saveTireSize(unsigned short newTireSize);

#define PRGSTATE_MPH   0x00
#define PRGSTATE_TSS   0x01
#define PRGSTATE_NOREQ 0xFF

void prgScreenMPH();
void prgScreenTSS();