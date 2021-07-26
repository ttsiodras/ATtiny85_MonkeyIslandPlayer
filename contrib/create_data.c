#include <stdio.h>

#include "monkey.h"

#define SPEAKER_PIN PB0
#define LED_PIN     PB1
#define BUTTON_PIN  PB2

void main()
{
    for(int song=0; song<sizeof(g_Melodies)/sizeof(g_Melodies[0]); song++) {
        char filename[128];
        snprintf(filename, sizeof(filename), "frequencies_%d.data", song);
        FILE *fp_freq = fopen(filename, "w");
        snprintf(filename, sizeof(filename), "delay_%d.data", song);
        FILE *fp_delay = fopen(filename, "w");
        for(int i=0; i<g_Melodies[song].totalNotes; i++) {
            int freqAndDurationData = g_Melodies[song].songData[i];
            int freq = g_Melodies[song].frequencyLUT[(freqAndDurationData>>8) & 0xFF ];
            int durationMS = g_Melodies[song].delayLUT[freqAndDurationData & 0xFF];
            fprintf(fp_freq, "%d\n", freq);
            fprintf(fp_delay, "%d\n", durationMS);
        }
        fclose(fp_delay);
        fclose(fp_freq);
    }
}
