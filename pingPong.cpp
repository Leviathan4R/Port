#include <stdio.h>
#include <iostream>
#include <thread>

#include "serialib.h"

static inline void moveToPos(int const & x, int const & y) {
    std::cout << "\e["<< (x+1) << ";" << (y+1) << "H" << std::flush;
    return;
}

static inline void printChar(char const & aCharacter, char const & aParams) {
    unsigned int printVal = aParams;
    std::cout << "\e[4" << (printVal & 0b111) << "m";
    printVal >>= 3;
    std::cout << "\e[3" << (printVal & 0b111) << "m";
    std::cout << aCharacter << "\e[0m" << std::flush;

    return;
}

static inline void savePos() {
    std::cout << "\n" << "\e[s" << std::flush;
    return;
}

static inline void loadPos() {
    std::cout << "\e[u" << std::endl;
    return;
}

static inline void hideCursor() {
    std::cout << "\e[?25l" << std::endl;
    return;
}

int main(int argc, char const *argv[]) {
    moveToPos(0,0);
    std::cout << "Press ENTER to start" << std::flush;
    int i;
    std::cin >> i; 
    hideCursor();

    moveToPos(0,0);
    for (size_t i = 0; i < 80; ++i) {
        printChar(' ', 0b00111111);
    }
/* 
    for (size_t i = 0; i < 40; ++i) {
        std::cout << std::endl;
    } */

    moveToPos(39, 0);
    for (size_t i = 0; i < 80; ++i){
        printChar(' ', 0b00111111);
    }
    savePos();

    constexpr int resRow = 40, resCol = 80;
    constexpr int minRow = 1, maxRow = 38, minCol = 1, maxCol = 78;
    int row = 200, col = 400;
    int movRow = 5, movCol = 10;
    while (true) {
        // Example of write
        moveToPos(row / resRow, col / resCol);
        printChar(' ', 0b00000000);

        if(row / resRow >= maxRow) {
            movRow = -abs(movRow);
        }
        else if (row / resRow <= minRow) {
            movRow = abs(movRow);
        }
        
        if(col / resCol >= maxCol) {
            movCol = -abs(movCol);
        }
        else if(col / resCol <= minCol) {
            movCol = abs(movCol);
        }
        
        // Update positions
        row += movRow;
        col += movCol;

        // Example of write
        moveToPos(row / resRow, col / resCol);
        printChar(' ', 0b00111111);
        usleep(5000);
    }
    


    // Example of clear
    moveToPos(20, 40);
    printChar(' ', 0b00000000);

    // Example of another write
    moveToPos(21, 39);
    printChar('o', 0b00010000);
    usleep(1000000);


    loadPos();
    return 0;
}
