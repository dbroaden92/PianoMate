#include "STM32L1xx.h"

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define HOME    1
#define PLAY    2
#define PAUSE   3

//------------------------------------------------------------------------------
// Structs
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
int state;
int song;
int mode;
long beat;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
void setup();
void homeState();
void playState();
void pauseState();
int checkSongSelect();
int checkModeSelect();
int checkStart();
int checkPause();
int checkStop();
void playBeat();

int main(void) {
    setup();

    while (1) {
        switch(state) {
            case HOME:
                homeState();
                break;
            case PLAY:
                playState();
                break;
            case PAUSE:
                pauseState();
                break;
            default:
                reset();
                state = HOME;
                homeState();
        }
    }

    return 0;
}

void setup() {

}

void homeState() {
    int selectedSong = checkSongSelect();
    if (selectedSong && selectedSong != song) {
        song = selectedSong;
    }

    int selectedMode = checkModeSelect();
    if (selectedMode && selectedMode != mode) {
        mode = selectedMode;
    }

    if (checkStart()) {
        state = PLAY;
    }
}

void playState() {
    if (checkStop()) {
        beat = 0;
        state = HOME;
    } else if (checkPause()) {
        state = PAUSE;
    } else {
        playBeat();
    }
}

void pauseState() {
    if (checkStop()) {
        beat = 0;
        state = HOME;
    } else if (checkStart()) {
        state = PLAY;
    }
}

int checkSongSelect() {

}

int checkModeSelect() {

}

int checkStart() {

}

int checkPause() {

}

int checkStop() {

}

void playBeat() {

}
