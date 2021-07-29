/**
 * As you can see in the #defines below, I connected:
 *
 * - The transistor's base to PB0 (via the 1K resistor)
 * - The LED to PB1 (via a 220Ohm resistor)
 * - ...and the pushbutton to PB3.
 *   The pin is pulled to GND via a 10K, but pulled to VCC when the button is pushed.
 *
 *                                     +--------+
 *        (PCINT5/~RESET/ADC0/dW) PB5  | 1    8 | VCC
 * (PCINT3/XTAL1/CLKI/~OC1B/ADC3) PB3  | 2    7 | PB2 (SCK/USCK/SCL/ADC1/T0/INT0/PCINT2)
 *  (PCINT4/XTAL2/CLKO/OC1B/ADC2) PB4  | 3    6 | PB1 (MISO/DO/AIN1/OC0B/OC1A/PCINT1)
 *                                GND  | 4    5 | PB0 (MOSI/DI/SDA/AIN0/OC0A/~OC1A/AREF/PCINT0)
 *                                     +--------+
 */

// #include <SendOnlySoftwareSerial.h>
// SendOnlySoftwareSerial mySerial(PB4);

#include "huffman_decoder.h"
#include "contrib/songs_in_huffman.h"

#define SPEAKER_PIN PB0
#define LED_PIN     PB1
#define BUTTON_PIN  PB3

unsigned long previousMicros;
unsigned song = 0xFFFF;

long onMicros, offMicros, periodMicros;
long onMicrosRemaining, offMicrosRemaining, periodsRemaining, silenceMicrosRemaining;

bool buttonIsPressed = false;
long microsWhenButtonWasPressed = 0;

long song_loops = 0;

enum StateType {
    PlayingON,
    PlayingOFF,
    Silence
} state;

void setup()
{
    // mySerial.begin(115200);
    pinMode(SPEAKER_PIN, OUTPUT);   
    pinMode(BUTTON_PIN, INPUT);   
    previousMicros = micros();
}

HuffmanDecoder hd_frequencies;
HuffmanDecoder hd_delays;

void loadSong();

void updateState()
{
    int freq = hd_frequencies.decode();
    // mySerial.println(freq);
    int durationMS = hd_delays.decode();
    if (freq == 0x7FFF || durationMS == 0x7FFF) {
        if (!song_loops)
            state = Silence;
        else {
            song_loops--;
            loadSong();
        }
        return;
    }
    if (freq != -1) {
        int volume = 60;
        periodMicros = 1000000/((long)freq);
        onMicros = periodMicros * volume/100;
        offMicros = periodMicros * (100-volume)/100;
        state = PlayingON;
        onMicrosRemaining = onMicros;
        offMicrosRemaining = offMicros;
        periodsRemaining = ((long)durationMS)*1000L/periodMicros;
        digitalWrite(SPEAKER_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
    } else {
        state = Silence;
        silenceMicrosRemaining = ((long)durationMS)*1000L;
        digitalWrite(SPEAKER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
    }
}

void loadSong()
{
    hd_frequencies.loadNewData(
        g_Melodies[song].frequencyData,
        g_Melodies[song].frequencyDataTotalBits,
        g_Melodies[song].huffmanFrequencyCodes);
    hd_delays.loadNewData(
        g_Melodies[song].delayData,
        g_Melodies[song].delayDataTotalBits,
        g_Melodies[song].huffmanDelayCodes);
    updateState();
}

void loop()
{
    unsigned long currentMicros = micros();
    if (digitalRead(BUTTON_PIN) == HIGH) {
        if (!buttonIsPressed) {
            buttonIsPressed = true;
            microsWhenButtonWasPressed = currentMicros;
        }
    } else {
        if (buttonIsPressed && ((currentMicros-microsWhenButtonWasPressed)>100000L)) {
            if (song == 0xFFFF) {
                song = 0;
                song_loops = g_Melodies[song].loops;
                loadSong();
            } else {
                song++;
                if (song >= sizeof(g_Melodies)/sizeof(g_Melodies[0]))
                    song = 0xFFFF;
                else {
                    song_loops = g_Melodies[song].loops;
                    loadSong();
                }
            }
            buttonIsPressed = false;
        }
    }

    if (song == 0xFFFF)
        return;

    int passedMicros = currentMicros-previousMicros;

    switch(state) {
    case Silence:
        silenceMicrosRemaining -= passedMicros;
        if (silenceMicrosRemaining < 0) {
            updateState();
        }
        break;
    case PlayingON:
        onMicrosRemaining -= passedMicros;
        if (onMicrosRemaining < 0) {
            onMicrosRemaining = onMicros;
            state = PlayingOFF;
            digitalWrite(SPEAKER_PIN, LOW);
            digitalWrite(LED_PIN, LOW);
        }
        break;
    case PlayingOFF:
        offMicrosRemaining -= passedMicros;
        if (offMicrosRemaining < 0) {
            offMicrosRemaining = offMicros;
            if (!periodsRemaining) {
                updateState();
            } else {
                periodsRemaining--;
                state = PlayingON;
                digitalWrite(SPEAKER_PIN, HIGH);
                digitalWrite(LED_PIN, HIGH);
            }
        }
    }
    previousMicros = currentMicros;
}
