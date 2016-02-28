
#include <Wire.h>
#include <ZX_Sensor.h>

#include <MIDI.h>

// Constants
const int ZX_ADDR = 0x10;  // ZX Sensor I2C address
const int MIDI_CHANNEL = 1;

// ----------------

#include <SoftwareSerial.h>
SoftwareSerial midiSerial(8,9);
MIDI_CREATE_INSTANCE(SoftwareSerial, midiSerial, MIDI);

ZX_Sensor zx_sensor = ZX_Sensor(ZX_ADDR);

void setup()
{
    uint8_t ver;

    // Initialize Serial port
    Serial.begin(9600);
  
    // Initialize ZX Sensor (configure I2C and read model ID)
    if ( zx_sensor.init() ) {
        Serial.println("ZX Sensor initialization complete");
    } else {
        Serial.println("Something went wrong during ZX Sensor init!");
    }
  
    // Read the model version number and ensure the library will work
    ver = zx_sensor.getModelVersion();
    if ( ver == ZX_ERROR ) {
        Serial.println("Error reading model version number");
    } else {
        Serial.print("Model version: ");
        Serial.println(ver);
    }
    if ( ver != ZX_MODEL_VER ) {
        Serial.print("Model version needs to be ");
        Serial.print(ZX_MODEL_VER);
        Serial.print(" to work with this library. Stopping.");
        while(1);
    }
  
    // Read the register map version and ensure the library will work
    ver = zx_sensor.getRegMapVersion();
    if ( ver == ZX_ERROR ) {
        Serial.println("Error reading register map version number");
    } else {
        Serial.print("Register Map Version: ");
        Serial.println(ver);
    }
    if ( ver != ZX_REG_MAP_VER ) {
        Serial.print("Register map version needs to be ");
        Serial.print(ZX_REG_MAP_VER);
        Serial.print(" to work with this library. Stopping.");
        while(1);
    }

    MIDI.begin(MIDI_CHANNEL);
}

#define MAXZ 100

boolean inrange = false;
uint8_t old_xx = 0xff;
uint8_t old_zz = 0xff;
int old_note = -1;
int old_velocity = -1;

int min_xx = 0xff;
int max_xx = 0;

void map_to_note(int xx, int zz, int & note, int & velocity)
{
    if (min_xx > xx)
        min_xx = xx;

    if (max_xx < xx)
        max_xx = xx;

    int range_xx = max_xx - min_xx;
    
    // Bail if we have no x range.
    if (range_xx <= 0) {
        note = 42;
        velocity = 0;
        return;
    }

    double nval = (double) (xx - min_xx) / (double) range_xx;
    note = 127 - (int)(nval * 127);

    double vval = (double) (zz) / (double) MAXZ;
    velocity = 127 - (int) (vval * 127);

#if defined(DEBUG)
    String msg = "pos: ";
    Serial.println(msg + xx + ", " + zz +
                   "\t note: " + note + ", " + velocity +
                   "\t min/max: " + min_xx + ", " + max_xx);
#endif
}

void loop()
{
    // Bail if no sensor data.
    if (!zx_sensor.positionAvailable())
        return;

    uint8_t xx = zx_sensor.readX();
    uint8_t zz = zx_sensor.readZ();

    // Bail if we've got an error.
    if (xx == ZX_ERROR && zz == ZX_ERROR)
        return;

    if (inrange && zz > MAXZ) {
        // NOTE OFF
        inrange = false;
#if defined (DEBUG)
        Serial.println("");
#endif
        if (old_note != -1) {
            MIDI.sendNoteOff(old_note, 0, MIDI_CHANNEL);
            old_note = -1;
        }
    }
    else if (zz <= MAXZ) {
        inrange = true;
        if (xx != old_xx || zz != old_zz) {
            // NOTE CHANGED
            int note;
            int velocity;
            map_to_note(xx, zz, note, velocity);
            if (note != old_note || velocity != old_velocity) {
                MIDI.sendNoteOff(old_note, 0, MIDI_CHANNEL);
                MIDI.sendNoteOn(note, velocity, MIDI_CHANNEL);
            }
            old_xx = xx;
            old_zz = zz;
            old_note = note;
            old_velocity = velocity;
        }
    }
}
