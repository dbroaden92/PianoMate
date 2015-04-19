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
    int endOfSong;
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
void resetSong(int index);
void changeState(int);
void changeSong(int);
void changeMode(int);
void playBeat(void);
void activateKeys(int* keyArr, int length);
void deactivateKeys(int* keyArr, int length);
void deactivateAllKeys(void);

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
void song1(int beat);

//------------------------------------------------------------------------------
// Main Loop
//------------------------------------------------------------------------------
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
                beat = 0;
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
    beat = 0;

    deactivateAllKeys();
}

void loadSongs() {
    songs = malloc(NUM_SONGS * sizeof(struct Song));

    songs[0].tempo = 5000;
    songs[0].beat = 1;
    songs[0].endOfSong = 0;
}

void  resetSong(int index) {
    songs[index].beat = 1;
    songs[index].endOfSong = 0;
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
            switch(songID) {
                case 0:
                    song1(songs[songID].beat);
                    break;
                default:
                    break;
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

void song1(int beat) {
    int nextBeat = -1, onKeys[10], offKeys[10], onLength = 0, offLength = 0, end = 0;
    switch(beat) {
        case 1:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 2;
            break;
        case 2:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 5;
            break;
        case 5:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 6;
            break;
        case 6:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 9;
            break;
        case 9:
            onKeys[0] = 0;
            onLength = 1;
            nextBeat = 10;
            break;
        case 10:
            offKeys[0] = 0;
            offLength = 1;
            nextBeat = 13;
            break;
        case 13:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 14;
            break;
        case 14:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 17;
            break;
        case 17:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 18;
            break;
        case 18:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 21;
            break;
        case 21:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 22;
            break;
        case 22:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 25;
            break;
        case 25:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 26;
            break;
        case 26:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 33;
            break;
        case 33:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 34;
            break;
        case 34:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 37;
            break;
        case 37:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 38;
            break;
        case 38:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 41;
            break;
        case 41:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 42;
            break;
        case 42:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 49;
            break;
        case 49:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 50;
            break;
        case 50:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 53;
            break;
        case 53:
            onKeys[0] = 7;
            onLength = 1;
            nextBeat = 54;
            break;
        case 54:
            offKeys[0] = 7;
            offLength = 1;
            nextBeat = 57;
            break;
        case 57:
            onKeys[0] = 7;
            onLength = 1;
            nextBeat = 58;
            break;
        case 58:
            offKeys[0] = 7;
            offLength = 1;
            nextBeat = 65;
            break;
        case 65:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 66;
            break;
        case 66:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 69;
            break;
        case 69:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 70;
            break;
        case 70:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 73;
            break;
        case 73:
            onKeys[0] = 0;
            onLength = 1;
            nextBeat = 74;
            break;
        case 74:
            offKeys[0] = 0;
            offLength = 1;
            nextBeat = 77;
            break;
        case 77:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 78;
            break;
        case 78:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 81;
            break;
        case 81:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 82;
            break;
        case 82:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 85;
            break;
        case 85:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 86;  
            break;
        case 86:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 89;
            break;
        case 89:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 90;
            break;
        case 90:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 93;
            break;
        case 93:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 94;
            break;
        case 94:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 97;
            break;
        case 97:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 98;
            break;
        case 98:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 101;
            break;
        case 101:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 102;
            break;
        case 102:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 105;
            break;
        case 105:
            onKeys[0] = 4;
            onLength = 1;
            nextBeat = 106;
            break;
        case 106:
            offKeys[0] = 4;
            offLength = 1;
            nextBeat = 109;
            break;
        case 109:
            onKeys[0] = 2;
            onLength = 1;
            nextBeat = 110;
            break;
        case 110:
            offKeys[0] = 2;
            offLength = 1;
            nextBeat = 113;
            break;
        case 113:
            onKeys[0] = 0;
            onLength = 1;
            nextBeat = 114;
            break;
        case 114:
            offKeys[0] = 0;
            offLength = 1;
            nextBeat = 128;
            break;
        case 128:
            end = 1;
            break;
        default:
            end = 1;
    }

    if (songID == 0) {
        activateKeys(onKeys, onLength);
        deactivateKeys(offKeys, offLength);
        if (end == 1) {
            songs[songID].endOfSong = 1;
        } else {
            songs[songID].beat = nextBeat;
        }
    }
}
