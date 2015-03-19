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

//------------------------------------------------------------------------------
// Structs
//------------------------------------------------------------------------------
struct Song {
    int tempo;
    int beat;
    int* onKeys;
    int onKeysLength;
    int* offKeys;
    int offKeysLength;
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
struct Song* songs;
int numSongs;

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
void reset(void);
void setup(void);
void loadSongs(void);
int resetSong(int index);
void playBeat(void);
void activateKeys(int* keyArr, int length);
void deactivateKeys(int* keyArr, int length);
void deactivateAllKeys(void);
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
        } else if (state != HOME && state != PAUSE) {
            reset();
            state = HOME;
        }
    }
}

//------------------------------------------------------------------------------
// Interrupt Handler Prototypes
//------------------------------------------------------------------------------
void EXTI0_IRQHandler(void) {
    if (state == HOME) {
        if (songID == numSongs - 1) {
            songID = 0;
        } else {
            songID++;
        }
    }

    GPIOA->ODR &= ~(0x00000030);
    GPIOA->ODR |= songID << 4;

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

    GPIOA->ODR &= ~(0x000000C0);
    GPIOA->ODR |= mode << 6;

    EXTI->PR |= (0x00000002);
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
}

void EXTI2_IRQHandler(void) {
    if (state == HOME || state == PAUSE) {
        if (state == HOME) {
            resetSong(songID);
            beat = -1;
        }
        state = PLAY;
    } else if (state == PLAY) {
        state = PAUSE;
    }

    GPIOA->ODR &= ~(0x00000300);
    GPIOA->ODR |= state << 8;

    EXTI->PR |= (0x00000004);
    NVIC_ClearPendingIRQ(EXTI2_IRQn);
}

void EXTI3_IRQHandler(void) {
    if (state == PLAY || state == PAUSE) {
        state = HOME;
    }

    GPIOA->ODR &= ~(0x00000300);
    GPIOA->ODR |= state << 8;

    EXTI->PR |= (0x00000008);
    NVIC_ClearPendingIRQ(EXTI3_IRQn);
}

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
void reset() {
    state = HOME;
    songID = 0;
    mode = 0;
    beat = -1;

    // Reset State LEDs
    GPIOA->ODR &= ~(0x00000300);
    GPIOA->ODR |= state << 8;

    // Reset Song ID LEDs
    GPIOA->ODR &= ~(0x00000030);
    GPIOA->ODR |= songID << 4;

    // Reset Mode LEDs
    GPIOA->ODR &= ~(0x000000C0);
    GPIOA->ODR |= mode << 6;
}

void setup() {
    // Ports
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

    // Variables
    reset();

    // Songs
    loadSongs();

    // Enable all interrupts
    __enable_irq();
}

void loadSongs() {
    static const int *empty[1] = {0};
    static const char *songNotes1[5][200] = {"1/0|", "5/1|", "9/2|", "13/|2", "17/1"};
    struct Song song1 = {
        50,
        0,
        (int*) empty,
        0,
        (int*) empty,
        0,
        0,
        (char**) songNotes1,
        0,
        5
    };
    struct Song newSongs[1];

    numSongs = 1;
    newSongs[0] = song1;
    songs = newSongs;
}

int  resetSong(int index) {
    if (index >= 0 && index <= numSongs - 1) {
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

void playBeat() {
    if ((beat / songs[songID].tempo) >= songs[songID].beat) {
        if (songs[songID].endOfSong) {
            deactivateAllKeys();
            state = HOME;
        } else {
            activateKeys(songs[songID].onKeys, songs[songID].onKeysLength);
            deactivateKeys(songs[songID].offKeys, songs[songID].offKeysLength);
            if (songs[songID].noteIndex >= songs[songID].notesLength) {
                songs[songID].endOfSong = 1;
                songs[songID].beat++;
            } else {
                int i;
                long beatNum;
                char line[sizeof(songs[songID].notes[songs[songID].noteIndex])], *tofree;
                char *beatNumStr, *onKeyStr, *offKeyStr;
                char **beatSplit, **keySplit, **onKeyArr, **offKeyArr;
                int onKeys[100], offKeys[100], onKeysLength, offKeysLength;
                strcpy(line, songs[songID].notes[songs[songID].noteIndex]);
                tofree = line;

                beatSplit = str_split(line, '/');
                beatNumStr = beatSplit[0];
                keySplit = str_split(beatSplit[1], '|');
                onKeyStr = keySplit[0];
                offKeyStr = keySplit[1];

                beatNum = strtol(beatNumStr, (char**) NULL, 10);
                onKeyArr = str_split(onKeyStr, ',');
                offKeyArr = str_split(offKeyStr, ',');

                i = 0;
                while (onKeyArr[i] != NULL) {
                    onKeys[i] = strtol(onKeyArr[i], (char**) NULL, 10);
                    i++;
                }
                onKeysLength = i;

                i = 0;
                while (offKeyArr[i] != NULL) {
                    offKeys[i] = strtol(offKeyArr[i], (char**) NULL, 10);
                    i++;
                }
                offKeysLength = i;

                songs[songID].beat = beatNum;
                songs[songID].onKeys = onKeys;
                songs[songID].onKeysLength = onKeysLength;
                songs[songID].offKeys = offKeys;
                songs[songID].offKeysLength = offKeysLength;
                songs[songID].noteIndex++;

                free(tofree);
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

char *strdup (const char *s) {
    char *d = malloc(strlen(s) + 1);
    if (d == NULL) {
        return NULL;
    }
    strcpy(d, s);
    return d;
}
