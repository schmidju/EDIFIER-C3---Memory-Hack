
#include <avr/io.h>
#include "edifier.h"

uint8_t valueToVolume(uint8_t value) {
    value = 63 - value;
    if (value <= 26) {
        return value >> 1;
    }
    else if (value <= 53) {
        return value - 13;
    }
    else {
        return 40 + ((value - 53) << 1); 
    }
}

uint8_t valueToInput(uint8_t value) {
    if (value & 0x01) {
        return INPUT_PC;
    }
    else {
        return INPUT_AUX;
    }
}

int8_t valueToBass(uint8_t value) {
    int8_t bass = 5 - (value & 0x1F);

    if (bass >= 0) {
        return bass << 1;
    }
    else {
        return bass >> 1;
    }
}

int8_t valueToTreb(uint8_t value) {
    value &= 0x0F;
    if (value <= 7) {
        return value - 7;
    }
    else {
        return 15 - value;
    }
}

void edi_decode_package(uint8_t* package, Edifier* edifier) {
    if (package[1] & 0xC0) {
        edifier->volume = valueToVolume(package[0]);
        edifier->input = valueToInput(package[5]);
        edifier->treb = valueToTreb(package[4]);
        edifier->bass = valueToBass(package[1]);        
    }
}