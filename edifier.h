#define INPUT_AUX   0
#define INPUT_PC    1

#define SYSTEM_MUTE 0
#define SYSTEM_ON 1

typedef struct {
    uint8_t volume : 7;
    uint8_t input : 1;
    int8_t treb : 6;
    int8_t bass : 6;
    uint8_t mute : 1;
} Edifier;

void edi_decode_package(uint8_t* package, Edifier* edifier);
void edi_clamp_settings(Edifier* settings);