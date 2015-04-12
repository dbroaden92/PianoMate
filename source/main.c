//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "STM32L1xx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define HOME        1
#define PLAY        2
#define PAUSE       3
#define NUM_SONGS   1

//------------------------------------------------------------------------------
// Structs
//------------------------------------------------------------------------------
struct Song {
    int tempo;
    int beat;
    int *onKeys;
    int onKeysLength;
    int *offKeys;
    int offKeysLength;
    int endOfSong;
    char **notes;
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
struct Song* songs;
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
void setup(void);
void reset(void);
void loadSongs(void);
void changeState(int);
void changeSong(int);
void changeMode(int);
int resetSong(int index);
void playBeat(void);
void activateKeys(int* keyArr, int length);
void deactivateKeys(int* keyArr, int length);
void deactivateAllKeys(void);

//------------------------------------------------------------------------------
// Utility Function Prototypes
//------------------------------------------------------------------------------
char** str_split(char* str, const char delim);
char *strdup(const char *s);
char *strcpy(char *dest, const char *src);
size_t strlen(const char *s);
char *strtok(char *str, const char *delim);

int main(void) {

    setup();

    while (1) {
        if (state == PLAY) {
            playBeat();
        } else if (state != HOME && state != PLAY && state != PAUSE) {
            reset();
        }
    }
}

//------------------------------------------------------------------------------
// Interrupt Handler Prototypes
//------------------------------------------------------------------------------
// Song Select Button
void EXTI0_IRQHandler(void) {
    if ((EXTI->IMR & EXTI_IMR_MR0) && (EXTI->PR & EXTI_PR_PR0)) {
        if (state == HOME) {
            if (songID == NUM_SONGS - 1) {
                changeSong(0);
            } else {
                changeSong(songID + 1);
            }
        }

        EXTI->PR |= EXTI_PR_PR0;
        NVIC_ClearPendingIRQ(EXTI0_IRQn);
    }
}

// Mode Select Button
void EXTI1_IRQHandler(void) {
    if ((EXTI->IMR & EXTI_IMR_MR1) && (EXTI->PR & EXTI_PR_PR1)) {
        if (state == HOME) {
            if (mode == 2) {
                changeMode(0);
            } else {
                changeMode(mode + 1);
            }
        }

        EXTI->PR |= EXTI_PR_PR1;
        NVIC_ClearPendingIRQ(EXTI1_IRQn);
    }
}

// Play/Pause Button
void EXTI2_IRQHandler(void) {
    if ((EXTI->IMR & EXTI_IMR_MR2) && (EXTI->PR & EXTI_PR_PR2)) {
        if (state == HOME || state == PAUSE) {
            if (state == HOME) {
                resetSong(songID);
                beat = -1;
            }
            changeState(PLAY);
        } else if (state == PLAY) {
            changeState(PAUSE);
        }

        EXTI->PR |= EXTI_PR_PR2;
        NVIC_ClearPendingIRQ(EXTI2_IRQn);
    }
}

// Stop Button
void EXTI3_IRQHandler(void) {
    if ((EXTI->IMR & EXTI_IMR_MR3) && (EXTI->PR & EXTI_PR_PR3)) {
        if (state == PLAY || state == PAUSE) {
            changeState(HOME);
            deactivateAllKeys();
        }

        EXTI->PR |= EXTI_PR_PR3;
        NVIC_ClearPendingIRQ(EXTI3_IRQn);
    }
}

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
void setup() {
    // Ports
    RCC->AHBENR |= 0x07; // Enable GPIOA, GPIOB, and GPIOC clocks

    // Inputs
    GPIOA->MODER &= ~(0x000000FF); // Clear PA0-3 mode (input)
    GPIOA->OSPEEDR &= ~(0x000000FF); // Clear PA0-3 speed
    GPIOA->OSPEEDR |= (0x00000055); // 25 MHz medium speed
    GPIOA->PUPDR &= ~(0x000000FF); // Clear PA0-3 pull-up/pull-down (no PuPd)

    // Outputs
    GPIOA->MODER &= ~(0x003FFF00); // Clear PA4-10 mode
    GPIOA->MODER |= (0x00155500); // Set PA4-10 as output
    GPIOA->OTYPER &= ~(0x000007F0); // Clear PA4-10 out type (push/pull)
    GPIOA->OSPEEDR &= ~(0x003FFF00); // Clear PA4-10 speed (2 MHz low speed)
    GPIOA->PUPDR &= ~(0x003FFF00); // Clear PA4-10 pull-up/pull-down (no PuPd)

    GPIOB->MODER &= ~(0xFFFFFFFF);
    GPIOB->MODER |= (0x55555555);
    GPIOB->OTYPER &= ~(0x0000FFFF);
    GPIOB->OSPEEDR &= ~(0xFFFFFFFF);
    GPIOB->PUPDR &= ~(0xFFFFFFFF);

    GPIOC->MODER &= ~(0xFFFFFFFF);
    GPIOC->MODER |= (0x55555555);
    GPIOC->OTYPER &= ~(0x0000FFFF);
    GPIOC->OSPEEDR &= ~(0xFFFFFFFF);
    GPIOC->PUPDR &= ~(0xFFFFFFFF);


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

    // Variables
    reset();

    // Songs
    loadSongs();

    // Clear Keys
    deactivateAllKeys();

    // Enable all interrupts
    __enable_irq();
}

void reset() {
    changeState(HOME);
    changeSong(0);
    changeMode(0);
    beat = -1;

    deactivateAllKeys();
}

void loadSongs() {
    int empty[1] = {0};
    static char** songNotes1;
    songNotes1 = malloc(sizeof(char*) * 15);

    songNotes1[0] = "|1|0,16,|,|";
    songNotes1[1] = "|2|,|0,16,|";
    songNotes1[2] = "|3|0,16,|,|";
    songNotes1[3] = "|4|,|0,16,|";
    songNotes1[4] = "|5|0,16,|,|";
    songNotes1[5] = "|6|,|0,16,|";
    songNotes1[6] = "|7|0,16,|,|";
    songNotes1[7] = "|8|,|0,16,|";
    songNotes1[8] = "|9|0,16,|,|";
    songNotes1[9] = "|10|,|0,16,|";

    songs = malloc(NUM_SONGS * sizeof(struct Song));

    songs[0].tempo = 50000;
    songs[0].beat = 0;
    songs[0].onKeys = empty;
    songs[0].onKeysLength = 0;
    songs[0].offKeys = empty;
    songs[0].offKeysLength = 0;
    songs[0].endOfSong = 0;
    songs[0].notes = songNotes1;
    songs[0].noteIndex = 0;
    songs[0].notesLength = 10;
}

int  resetSong(int index) {
    if (index >= 0 && index <= NUM_SONGS - 1) {
        int empty[1] = {0};
        songs[index].beat = 0;
        songs[index].onKeys = empty;
        songs[index].onKeysLength = 0;
        songs[index].offKeys = empty;
        songs[index].offKeysLength = 0;
        songs[index].endOfSong = 0;
        songs[index].noteIndex = 0;

        return 1;
    }

    return 0;
}

void changeState(int nextState) {
    if (nextState == HOME || nextState == PLAY || nextState == PAUSE) {
        state = nextState;
        GPIOA->ODR &= ~(0x00000300);
        GPIOA->ODR |= state << 8;
    }
}

void changeSong(int nextSongID) {
    if (nextSongID >= 0 && nextSongID < NUM_SONGS) {
        songID = nextSongID;

        GPIOA->ODR &= ~(0x00000030);
        GPIOA->ODR |= songID << 4;
    }
}

void changeMode(int nextMode) {
    if (nextMode == 0 || nextMode == 1 || nextMode == 2) {
        mode = nextMode;

        GPIOA->ODR &= ~(0x000000C0);
        GPIOA->ODR |= mode << 6;
    }
}

void playBeat() {
    if (((double) beat / songs[songID].tempo) >= songs[songID].beat) {
        if (songs[songID].endOfSong) {
            deactivateAllKeys();
            changeState(HOME);
        } else {
            activateKeys(songs[songID].onKeys, songs[songID].onKeysLength);
            deactivateKeys(songs[songID].offKeys, songs[songID].offKeysLength);
            if (songs[songID].noteIndex >= songs[songID].notesLength) {
                songs[songID].endOfSong = 1;
                songs[songID].beat++;
            } else {
                int i;
                long beatNum;
                char *line, *beatNumStr, *onKeyStr, *offKeyStr;
                char **lineSplit, **onKeyArr, **offKeyArr;
                int *onKeys, *offKeys, onKeysLength, offKeysLength;
                free(songs[songID].onKeys);
                free(songs[songID].offKeys);
                line = malloc(sizeof(char) * 25);
                strcpy(line, songs[songID].notes[songs[songID].noteIndex]);

                lineSplit = str_split(line, '|');
                free(line);
                beatNumStr = lineSplit[0];
                onKeyStr = lineSplit[1];
                offKeyStr = lineSplit[2];

                beatNum = strtol(beatNumStr, (char**) NULL, 10);
                free(beatNumStr);

                onKeyArr = str_split(onKeyStr, ',');
                free(onKeyStr);

                offKeyArr = str_split(offKeyStr, ',');
                free(offKeyStr);
                free(lineSplit);

                onKeys = malloc(sizeof(int) * 10);
                offKeys = malloc(sizeof(int) * 10);

                i = 0;
                while (onKeyArr[i] != NULL) {
                    onKeys[i] = strtol(onKeyArr[i], (char**) NULL, 10);
                    i++;
                }
                onKeysLength = i;
                free(onKeyArr);

                i = 0;
                while (offKeyArr[i] != NULL) {
                    long testInt;
                    testInt = strtol(offKeyArr[i], (char**) NULL, 10);
                    offKeys[i] = testInt;
                    i++;
                }
                offKeysLength = i;
                free(offKeyArr);

                songs[songID].beat = beatNum;
                songs[songID].onKeys = onKeys;
                songs[songID].onKeysLength = onKeysLength;
                songs[songID].offKeys = offKeys;
                songs[songID].offKeysLength = offKeysLength;
                songs[songID].noteIndex++;
            }
        }
    }
    beat++;
}

void activateKeys(int* keyArr, int length) {
    uint32_t activateB = (0x00000000);
    uint32_t activateC = (0x00000000);
    int i;
    for (i = 0; i < length; i++) {
        int key = keyArr[i];
        if (key >= 0 && key <=15) {
            activateB |= 1 << key;
        } else if (key >= 16 && key <= 31) {
            activateC |= 1 << (key - 16);
        }
    }

    GPIOB->ODR |= activateB;
    GPIOC->ODR |= activateC;
}

void deactivateKeys(int* keyArr, int length) {
    uint32_t deactivateB = (0x00000000);
    uint32_t deactivateC = (0x00000000);
    int i;
    for (i = 0; i < length; i++) {
        int key = keyArr[i];
        if (key >= 0 && key <=15) {
            deactivateB |= 1 << key;
        } else if (key >= 16 && key <= 31) {
            deactivateC |= 1 << (key - 16);
        }
    }

    GPIOB->ODR &= ~(deactivateB);
    GPIOC->ODR &= ~(deactivateC);
}

void deactivateAllKeys() {
    GPIOB->ODR &= ~(0x0000FFFF);
    GPIOC->ODR &= ~(0x0000FFFF);
}

char** str_split(char* a_str, const char a_delim) {
    char** result = 0;
    size_t count = 0;
    char* tmp = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    // Count how many elements will be extracted.
    while (*tmp) {
        if (a_delim == *tmp) {
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

    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token) {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }

    return result;
}

char *strdup (const char *s) {
    char *d = malloc(strlen(s) + 1);
    if (d == NULL) {
        return NULL;
    }
    strcpy(d, s);
    return d;
}

char *strcpy(char *dest, const char *src) {
    char *saved = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
    return saved;
}
