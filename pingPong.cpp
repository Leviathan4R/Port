#include <assert.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <thread>
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

#include "serialib.h"

#define SERIAL_PORT_OUT "COM6"

static inline int moveToPos(char const& x, char const& y) {
    std::cout << "\e[" << (x + 1) << ";" << (y + 1) << "H" << std::flush;
    return 0;
}

static inline int printChar(char const& aCharacter, char const& aParams) {
    unsigned int printVal = aParams;
    std::cout << "\e[4" << (printVal & 0b111) << "m";
    printVal >>= 3;
    std::cout << "\e[3" << (printVal & 0b111) << "m";
    std::cout << aCharacter << "\e[0m" << std::flush;
    return 0;
}

static inline int _printChar(serialib& aSerial, char const& aOper, char const& aCharacter, char const& aParams, size_t aCount = 1) {
    unsigned char message[4] = {aOper, aCharacter, aParams, 'l'};
    unsigned char retVal[2] = { 0 };
    unsigned int bytesWritten = 0;
    int tries = 0;

    aSerial.flushReceiver();

    for (size_t i = 0; i < aCount; ++i) {
        aSerial.writeBytes(message, 4, &bytesWritten);
        if (bytesWritten != 4) {
            if(!aSerial.getInQueStat()) {
                return -1;
            }
            else {
                --i;
                Sleep(1);
                if(tries > 5) {
                    return -1;
                }
                ++tries;
                continue;
            }

            tries = 0;
        }
    }

    while(aSerial.getOutQueStat() != 0) {
        Sleep(1);
    }

    for (size_t i = 0; i < aCount; ++i)
    {
        if (aSerial.getInQueStat() < 2) {
            return -2;
        }
        if (aSerial.readBytes(retVal, 2, 0, 0) != 2) {
            return -2;
        }
        if (retVal[0] != 'A' || retVal[1] != aOper) {
            return -3;
        }
    }
    
    aSerial.flushReceiver();

    return 0;
}

static inline int printChar(serialib& aSerial, char const& aCharacter, char const& aParams, size_t aCount = 1) {
    return _printChar(aSerial, 'c', aCharacter, aParams, aCount);
}

static inline int moveToPos(serialib& aSerial, char const& x, char const& y) {
    return _printChar(aSerial, 'p', x, y, 1);
}

static inline int clrScreen(serialib& aSerial) {
    unsigned int aString[4] = {'e', 127, 127, 'l'};
    unsigned int bytesWritten = 0;
    aSerial.writeBytes(aString, 4, &bytesWritten);
    if (bytesWritten != 4)
        return -1;

    while(aSerial.getOutQueStat())
        Sleep(1);

    aSerial.flushReceiver();

    return 0;
}

static inline void savePos() {
    std::cout << "\n"
              << "\e[s" << std::flush;
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
/*
class pingPong {
    serialib serial_out;
    const int resRow = 40, resCol = 80;
    const int minRow = 1, maxRow = 38, minCol = 1, maxCol = 78;
    int row = 200, col = 400;
    int movRow = 5, movCol = 10;
public:
    pingPong(/* args );
    ~pingPong();

    inline char openDevice(const char *Device, const unsigned int Bauds,
                    SerialDataBits Databits = SERIAL_DATABITS_8,
                    SerialParity Parity = SERIAL_PARITY_NONE,
                    SerialStopBits Stopbits = SERIAL_STOPBITS_1) {
                        return serial_out.openDevice(Device, Bauds, Databits, Parity, Stopbits);
                    }

    inline int init() {
        moveToPos(0,0);
        for (size_t i = 0; i < 80; ++i) {
            printChar(' ', 0b00111111);
        }
    /*
        for (size_t i = 0; i < 40; ++i) {
            std::cout << std::endl;
        }

        moveToPos(39, 0);
        for (size_t i = 0; i < 80; ++i){
            printChar(' ', 0b00111111);
        }
        savePos();
    }

    inline int move() {
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
    }
};*/

int main(int argc, char const* argv[]) {
    serialib serial_out;

    int error = 0;
    
    // std::cin >> error;
    error = serial_out.openDevice(SERIAL_PORT_OUT, 9600);
    if (error != 1) {
        std::cerr << "Nepodarilo sa pripojit v prvom kroku!" << std::endl;
        std::cin >> error;
        return -1;
    }
    serial_out.closeDevice();

    error = serial_out.openDevice(SERIAL_PORT_OUT, 115200);

    if (error != 1) {
        std::cerr << "Nepodarilo sa pripojit v druhom kroku!" << std::endl;
        std::cin >> error;
        return -1;
    }

    std::cout << "velkost I / O bufferu: " << serial_out.getInQueMax() << "/" << serial_out.getOutQueMax() << std::endl;

    std::cout << "Vystupny buffer: " << serial_out.getOutQueStat() << std::endl;

    constexpr char farbaBiela = (char)0b11111111;
    constexpr char farbaCierna = 0;

    moveToPos(serial_out, 0, 0);
    
    error = printChar(serial_out, ' ', farbaBiela, 80);
    if (error) 
        std::cerr << "chyba v zapisani" << std::endl;
    
    for (size_t u = 1; u < 39; ++u) {
        error = printChar(serial_out, ' ', farbaCierna, 80);
        if (error) 
            std::cerr << "chyba v zapisani" << std::endl;
    };

    moveToPos(serial_out, 0, 39);
    error = printChar(serial_out, ' ', farbaBiela, 80);
    if (error) 
        std::cerr << "chyba v zapisani" << std::endl;

    unsigned int realRow = 1, realCol = 1;
    constexpr unsigned int resRow = 1, resCol = 1;
    constexpr unsigned int minRow = 1, maxRow = 38, minCol = 1, maxCol = 78;
    unsigned int row = 1, col = 1;
    int movRow = 1, movCol = 1;
    while (error == 0) {
        // Odrazy od stien
        if (realRow >= maxRow) {
            movRow = -abs(movRow);
        } else if (realRow <= minRow) {
            movRow = abs(movRow);
        }

        if (realCol >= maxCol) {
            movCol = -abs(movCol);
        } else if (realCol <= minCol) {
            movCol = abs(movCol);
        }

        // Update positions
        row += movRow;
        col += movCol;

        // We dont need to write in every cycle
        if (realRow != (row / resRow) || realCol != (col / resCol)) {
            error = moveToPos(serial_out, realCol, realRow);    // Stores the previous positions
            if (error < 0) 
                return -1;
            error = printChar(serial_out, ' ', farbaCierna);
            if (error < 0) 
                return -1;

            realRow = (row / resRow);
            realCol = (col / resCol);

            // Example of write
            error = moveToPos(serial_out, realCol, realRow);    // Stores the new positions
            if (error < 0) 
                return -1;
            error = printChar(serial_out, ' ', farbaBiela);
            if (error < 0) 
                return -1;
        }

        if(realRow != row || realCol != col) {
            return -1;
        }

#if defined(_WIN32) || defined(_WIN64)
        Sleep(50);
#elif defined(__linux__)
        usleep(20000);
#endif
    }

    if (error) 
        std::cerr << "chyba v zapisani" << std::endl;

    std::cin >> error;
    return 0;
}
