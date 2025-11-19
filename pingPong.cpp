#include <assert.h>
#include <stdio.h>
#include <deque>
#include <exception>

#include <iostream>
#include <string>
#include <thread>
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

#include "serialib.h"

#define SERIAL_PORT_OUT "\\\\?\\COM10"

/*
X: 30-40 death zone
*/

constexpr char farbaBiela = (char)0b11111111;
constexpr char farbaCierna = 0b00000000;

struct TPosChar {
    char iPosX;
    char iPosY;
    char iChar;
    char iAtr;
};

class pingPong
{
    std::deque<TPosChar> iCheck_buffer;
    size_t iCheckCount;
    serialib iSerial_out;

    char iPosX, iPosY, iPrevX, iPrevY;
public:
    pingPong(const char* aPort, char aPosX, char aPosY) : iCheck_buffer(), iSerial_out(), iPosX(aPosX), iPosY(aPosY), iCheckCount(0), iPrevX(0), iPrevY(0) {
        // std::cin >> error;
        char error = iSerial_out.openDevice(aPort, 9600);
        if (error != 1) {
            throw std::runtime_error("Nepodarilo sa pripojit k zbernici!");
        }
        iSerial_out.closeDevice();

        error = iSerial_out.openDevice(aPort, 115200);

        if (error != 1) {
            throw std::runtime_error("Nepodarilo sa pripojit k zbernici!");
        }

        iSerial_out.flushInQue();
    }
    ~pingPong() {};

    void writePosChar(TPosChar & aOper) {
        // Write to iCheck_buffer
        iCheck_buffer.push_back(aOper);
    }

    void writePosChar(char aChar, char aAtr) {
        iCheck_buffer.push_back({iPosX, iPosY, aChar, aAtr});
    }

    // automatically writes blank white character
    void writePosChar() {
        iCheck_buffer.push_back({iPosX, iPosY, ' ', farbaBiela});
    }

    // basically writes blank black character at current position
    void clearPosChar() {
        iCheck_buffer.push_back({iPosX, iPosY, ' ', farbaCierna});
    }

    // Starts the checking and writing process
    void flushBuffer() {
        char writeBuffer[4] = { };
        unsigned int bytesWritten = 4;

        if (iSerial_out.getInQueStat() != iCheckCount * 4) {
            // leave all bytes intact, we need repairs
            iSerial_out.flushInQue();

            TPosChar iter;
            char readBuffer[2] = {};
            writeBuffer[3] = 'l';

            for (size_t i = 0; i < iCheckCount; ++i)
            {
                iter = iCheck_buffer.front();

                writeBuffer[0] = 'p';
                writeBuffer[1] = iter.iPosX;
                writeBuffer[2] = iter.iPosY;

                do {
                    iSerial_out.writeBytes(writeBuffer, 4, &bytesWritten);
                    if(bytesWritten != 4) {
                        continue;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    if(iSerial_out.getInQueStat() < 2) {
                        iSerial_out.flushInQue();
                        continue;
                    }
                    iSerial_out.readBytes(readBuffer, 2, 0, 0);
                    iSerial_out.flushInQue();

                } while(readBuffer[0] != 'A' || readBuffer[1] != 'p');

                writeBuffer[0] = 'c';
                writeBuffer[1] = iter.iChar;
                writeBuffer[2] = iter.iAtr;

                do {
                    iSerial_out.writeBytes(writeBuffer, 4, &bytesWritten);
                    if(bytesWritten != 4) {
                        continue;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    if(iSerial_out.getInQueStat() < 2) {
                        iSerial_out.flushInQue();
                        continue;
                    }
                    iSerial_out.readBytes(readBuffer, 2, 0, 0);
                    iSerial_out.flushInQue();

                } while(readBuffer[0] != 'A' || readBuffer[1] != 'c');

                iCheck_buffer.pop_front();
            }
            iCheckCount = 0;
        }
        else {
            // If we received everything we can check if the data are valid

            for (size_t i = 0; i < iCheckCount; ++i) {
                if(iSerial_out.getInQueStat() < 4) {
                    throw std::runtime_error("Not expected!");
                }

                iSerial_out.readBytes(writeBuffer, 4, 0, 0);

                // error checking
                if(writeBuffer[0] != 'A' || writeBuffer[1] != 'p' || writeBuffer[2] != 'A' || writeBuffer[3] != 'c') {
                    // Invalid return value detected repeat the write
                    iCheck_buffer.push_back(iCheck_buffer.front());
                }
                iCheck_buffer.pop_front();
            }
            iCheckCount = 0;
        }

        writeBuffer[3] = 'l';
        for (auto& iter: iCheck_buffer)
        {
            writeBuffer[0] = 'p';
            writeBuffer[1] = iter.iPosX;
            writeBuffer[2] = iter.iPosY;

            iSerial_out.writeBytes(writeBuffer, 4, &bytesWritten);
            if(bytesWritten != 4) {
                break;
            }

            writeBuffer[0] = 'c';
            writeBuffer[1] = iter.iChar;
            writeBuffer[2] = iter.iAtr;

            iSerial_out.writeBytes(writeBuffer, 4, &bytesWritten);
            if(bytesWritten != 4) {
                break;
            }

            ++iCheckCount;
        }
    }

    // calculates new X and Y positions
    void newPos() {
        // remember previous values
        iPrevX = iPosX;
        iPrevY = iPosY;

        ++iPosX;
        if(iPosX == 80) {
            iPosX = 0;
            ++iPosY;
            if(iPosY == 40) {
                iPosY = 0;
            }
        }
    }
};


/*
Funguje! :)
int main(int argc, char const* argv[]) {
    char error;
    serialib serial_out;

    error = serial_out.openDevice(SERIAL_PORT_OUT, 115200);

    if (error != 1) {
        throw std::runtime_error("Nepodarilo sa pripojit k zbernici!");
    }

    serial_out.flushInQue();
    char buffer[4] = {'p', 0, 0, 'l'};
    unsigned int bytesWritten;

    serial_out.writeBytes(buffer, 4, &bytesWritten);
    if(bytesWritten != 4) {
        return -1;
    }
    buffer[0] = 'c';
    buffer[1] = ' ';
    buffer[2] = farbaCierna;
    for (size_t i = 0; i < 80*40 - 1; ++i) {
        
        serial_out.writeBytes(buffer, 4, &bytesWritten);

        if (bytesWritten != 4) {
            break;
        }
    }
    buffer[2] = farbaBiela;
    serial_out.writeBytes(buffer, 4, &bytesWritten);
}*/


int moveToPos(serialib & aSerial, char const & x, char const & y) {
    char aString[4] = {'p', x, y, 'l'};
    unsigned int bytesWritten = 0;
    aSerial.writeBytes(aString, 4, &bytesWritten);
    if(bytesWritten != 4) 
        return -1;

    aSerial.readBytes(aString, 2, 10);

    // TODO: controla co vrati zbernica

    return 0;
}

int printChar(serialib & aSerial, char const & aCharacter, char const & aParams) {
    char aString[4] = {'c', aCharacter, aParams, 'l'};
    unsigned int bytesWritten = 0;
    aSerial.writeBytes(aString, 4, &bytesWritten);
    if(bytesWritten != 4) 
        return -1;

    aSerial.readBytes(aString, 2, 10);

    // TODO: controla co vrati zbernica
    
    return 0;
}

int clrScreen(serialib & aSerial) {
    unsigned char aString[4] = {'e', 127, 127, 'l'};
    unsigned int bytesWritten = 0;
    aSerial.writeBytes(aString, 4, &bytesWritten);
    if(bytesWritten != 4) 
        return -1;

    aSerial.readBytes(aString, 2, 10);

    if(aString[0] != 'A' || aString[1] != 'e') 
        return -2;
    
    return 0;
}

void printChar(char const & aCharacter, char const & aParams) {
    unsigned int printVal = aParams;
    std::cout << "\e[4" << (printVal & 0b111) << "m";
    printVal >>= 3;
    std::cout << "\e[3" << (printVal & 0b111) << "m";
    std::cout << aCharacter << "\e[0m" << std::flush;
    return;
}



void savePos() {
    std::cout << "\n" << "\e[s" << std::flush;
    return;
}

void loadPos() {
    std::cout << "\e[u" << std::endl;
    return;
}

void hideCursor() {
    std::cout << "\e[?25l" << std::endl;
    return;
}

int main(int argc, char const *argv[]) {
    serialib serial_out;

    char error = serial_out.openDevice(SERIAL_PORT_OUT, 115200);
    
    if (error != 1) {
        std::cerr << "Nepodarilo sa pripojit v druhom kroku!" << std::endl;
        std::cin >> error;
        return -1;
    }

    char farbaBiela = (char)0b11111111;
    char farbaCierna = 0;


    moveToPos(serial_out, 0, 0);
    size_t i = 0, u = 0;
    for (; i < 80; ++i) {
        printChar(serial_out, ' ', farbaBiela);
    }

    for (u = 1; u < 39; ++u) {  
        for (i = 0; i < 80; ++i) {
            printChar(serial_out, ' ', farbaCierna);
        }
    }
    

    moveToPos(serial_out, 0, 39);
    for (i = 0; i < 80; ++i){
        printChar(serial_out, ' ', farbaBiela);
    }

    char realRow = 0, realCol = 0;
    constexpr char resRow = 40, resCol = 80;
    constexpr int minRow = 1, maxRow = 38, minCol = 1, maxCol = 78;
    int row = 200, col = 400;
    int movRow = 20, movCol = 40;
    while (true) {
        // Example of write
        realRow = row / resRow;
        realCol = col / resCol;
        error = moveToPos(serial_out, realCol, realRow);
        if (error < 0) {
            std::cout << "Problem! movePos - delete, " << error << std::endl;
            break;
        }
        error = printChar(serial_out, ' ', farbaCierna);
        if (error < 0) {
            std::cout << "Problem! print - delete, " << error << std::endl;
            break;
        }

        // Odrazy od stien
        if(realRow >= maxRow) {
            movRow = -abs(movRow);
        }
        else if (realRow <= minRow) {
            movRow = abs(movRow);
        }
        
        if(realCol >= maxCol) {
            movCol = -abs(movCol);
        }
        else if(realCol <= minCol) {
            movCol = abs(movCol);
        }
        
        // Update positions
        row += movRow;
        col += movCol;

        realRow = (char)(row / resRow);
        realCol = (char)(col / resCol);

        // Example of write
        error = moveToPos(serial_out, realCol, realRow);
        if (error < 0) {
            std::cout << "Problem! movePos - write, " << error << std::endl;
            break;
        }
        error = printChar(serial_out, ' ', farbaBiela);
        if (error < 0) {
            std::cout << "Problem! print - write, " << error << std::endl;
            break;
        }
        Sleep(2);
    }

    std::cin >> error;
    return 0;
}
