//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "STM32L1xx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define HOME        1
#define PLAY        2
#define PAUSE       3
#define NUM_SONGS   4

//------------------------------------------------------------------------------
// Structs
//------------------------------------------------------------------------------
struct Song {
    int tempo;
    int beat;
    int* onKeys;
    int* offKeys;
    int endOfSong;  
    char** notes;
    int noteIndex;
    int notesLength;
};

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
int state;
int songID;
int mode;
long beat;
struct Song songs[NUM_SONGS];

//------------------------------------------------------------------------------
// Interrupt Handler Prototypes
//------------------------------------------------------------------------------
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
void setup();
void playBeat();
void activateKeys(int* keys);
void deactivateKeys(int* keys);
void deactivateAllKeys();
char** str_split(char* str, const char delim);

int main(void) {
    setup();

    while (1) {
        if (state == PLAY) {
            playBeat();
        } else if (state != HOME && state != PAUSE) {
            reset();
            state = HOME;
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
// Interrupt Handler Prototypes
//------------------------------------------------------------------------------
void EXTI0_IRQHandler(void) {
    if (state == HOME) {
        if (songID == NUM_SONGS - 1) {
            songID = 0;
        } else {
            songID++;
        }
    }

    EXTI->PR |= (0x00000001);
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
}

void EXTI1_IRQHandler(void) {
    if (state == HOME) {
        if (mode == 2) {
            mode = 0;
        } else {
            mode++;
        }
    }

    EXTI->PR |= (0x00000002);
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
}

void EXTI2_IRQHandler(void) {
    if (state == HOME || state == PAUSE) {
        state = PLAY;
    } else if (state == PLAY) {
        state = PAUSE;
    }

    EXTI->PR |= (0x00000004);
    NVIC_ClearPendingIRQ(EXTI2_IRQn);
}

void EXTI3_IRQHandler(void) {
    if (state == PLAY || state == PAUSE) {
        beat = -1;
        state = HOME;
    }

    EXTI->PR |= (0x00000008);
    NVIC_ClearPendingIRQ(EXTI3_IRQn);
}

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
void setup() {
    RCC->AHBENR |= 0x07; // Enable GPIOA, GPIOB, and GPIOC clocks

    // Inputs
    GPIOA->MODER &= ~(0x000000FF); // Clear PA0-3 mode (input)
    GPIOA->OSPEEDR &= ~(0x000000FF); // Clear PA0-3 speed
    GPIOA->OSPEEDR |= (0x000000AA); // 50 MHz fast speed
    GPIOA->PUPDR &= ~(0x000000FF); // Clear PA0-3 pull-up/pull-down (no PuPd)

    // Outputs
    GPIOA->MODER &= ~(0x003FFF00); // Clear PA4-10 mode
    GPIOA->MODER |= (0x00155500); // Set PA4-10 as output
    GPIOA->OTYPER &= ~(0x000007F0); // Clear PA4-10 out type (push/pull)
    GPIOA->OSPEEDR &= ~(0x003FFF00); // Clear PA4-10 speed (2 MHz low speed)
    GPIOA->PUPDR &= ~(0x003FFF00); // Clear PA4-10 pull-up/pull-down (no PuPd)

    // Configure Interrupts
    SYSCFG->EXTICR[0] &= ~(0xFFFF);
    EXTI->RTSR &= ~(0x0000000F);
    EXTI->FTSR &= ~(0x0000000F);
    EXTI->FTSR |= (0x0000000F);
    EXTI->IMR &= ~(0x0000000F);
    EXTI->IMR |= (0x0000000F);
    EXTI->PR |= (0x0000000F);
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
    NVIC_ClearPendingIRQ(EXTI2_IRQn);
    NVIC_ClearPendingIRQ(EXTI3_IRQn);

    // Enable all interrupts
    __enable_irq();
}

void playBeat() {
    if ((beat / songs[songID].tempo) >= songs[songID].beat) {
        if (songs[songID].endOfSong) {
            deactivateAllKeys();
            beat = -2;
            state = HOME;
        } else {
            activateKeys(songs[songID].onKeys);
            deactivateKeys(songs[songID].offKeys);
            if (songs[songID].noteIndex >= songs[songID].notesLength - 1) {
                songs[songID].endOfSong = 1;
                songs[songID].beat++;
            } else {
                songs[songID].noteIndex++;
                char line[sizeof(songs[songsID].notes[songs[songID].noteIndex])], *tofree;
                char* beatNumStr, onKeyStr, offKeyStr;
                strcpy(line, songs[songID].notes[songs[songID].noteIndex]);
                tofree = line;

                beatNumStr = strsep(&line, "/");
                onKeyStr = strsep(&line, "|");
                offKeyStr = line;

                long beatNum;
                char** onKeyArr, offKeyArr;
                int onKeys[100], offKeys[100];
                beatNum = strtol(beatNumStr, (char**) NULL, 10);
                onKeyArr = str_split(onKeyStr, ",");
                offKeyArr = str_split(offKeyStr, ",");

                int i = 0;
                while (onKeyArr[i] != NULL) {
                    onKeys[i] = strtol(onKeyArr[i], (char**) NULL, 10);
                    i++;
                }

                i = 0;
                while (offKeyArr[i] != NULL) {
                    offKeys[i] = strtol(offKeyArr[i], (char**) NULL, 10);
                    i++;
                }

                songs[songID].beat = beatNum;
                songs[songID].onKeys = onKeys;
                songs[songID].offKeys = offKeys;

                free(tofree);
            }
        }
    }
    beat++;
}

void activateKeys(int* keys) {

}

void deactivateKeys(int* keys) {

}

void deactivateAllKeys() {

}

char** str_split(char* a_str, const char a_delim) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    // Count how many elements will be extracted.
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    // Add space for trailing token.
    count += last_comma < (a_str + strlen(a_str) - 1);

    // Add space for terminating null string so caller
    // knows where the list of returned strings ends.
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
