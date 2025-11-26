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
constexpr char farbaKontrast = (char)0b00000111;
constexpr char farbaCierna = 0b00000000;
constexpr char padSize = 6;
constexpr char minX = 0, maxX = 79, minY = 0, maxY = 39;
constexpr int posResX = 100 * 80 - 1, posResY = 100 * 40 - 1;

struct TPosChar
{
    char iPosX;
    char iPosY;
    char iChar;
    char iAtr;
};

int clrScreen(serialib &aSerial)
{
    unsigned char aString[4] = {'e', 127, 127, 'l'};
    unsigned int bytesWritten = 0;
    aSerial.writeBytes(aString, 4, &bytesWritten);
    if (bytesWritten != 4)
        return -1;

    aSerial.readBytes(aString, 2, 10);

    if (aString[0] != 'A' || aString[1] != 'e')
        return -2;

    return 0;
}

char getKeyDownLeft() {
    // detect keyboard
    if((GetKeyState('W') & WM_KEYDOWN) && (GetKeyState('S') & WM_KEYDOWN)) {
        // NOP
    }
    else if ((GetKeyState('W') & WM_KEYDOWN)) {
        return -1;
    }
    else if ((GetKeyState('S') & WM_KEYDOWN)) {
        return 1;
    }

    return 0;
}

char getKeyDownRight() {
    // detect keyboard
    if((GetKeyState(VK_UP) & WM_KEYDOWN) && (GetKeyState(VK_DOWN) & WM_KEYDOWN)) {
        // NOP
    }
    else if ((GetKeyState(VK_UP) & WM_KEYDOWN)) {
        return -1;
    }
    else if ((GetKeyState(VK_DOWN) & WM_KEYDOWN)) {
        return 1;
    }

    return 0;
}

class pingPong
{
    std::deque<TPosChar> iCheck_buffer;
    size_t iCheckCount;
    serialib iSerial_out;

    char iScoreLeft = 0, iScoreRight = 0;

    char iPadLeft = 19, iPadRight = 19;
    char iScrPosX, iScrPosY, iPrevX, iPrevY;
    int iRealX, iRealY;
    int iMoveX, iMoveY;

    int moveToPos(char const &aX, char const &aY)
    {
        char aString[4] = {'p', aX, aY, 'l'};
        unsigned int bytesWritten = 0;
        iSerial_out.writeBytes(aString, 4, &bytesWritten);
        if (bytesWritten != 4)
            return -1;

        iSerial_out.readBytes(aString, 2, 10, 0);

        // TODO: controla co vrati zbernica

        return 0;
    }

    int printChar(char const &aCharacter, char const &aParams)
    {
        char aString[4] = {'c', aCharacter, aParams, 'l'};
        unsigned int bytesWritten = 0;
        iSerial_out.writeBytes(aString, 4, &bytesWritten);
        if (bytesWritten != 4)
            return -1;

        iSerial_out.readBytes(aString, 2, 10, 0);

        // TODO: controla co vrati zbernica

        return 0;
    }

    void movePad(char &aPad, char aMove)
    {
        char x = (&aPad == &iPadLeft ? minX : maxX);

        if (aMove > 0)
        {
            if (aPad <= minY + 1)
                return;

            --aPad;

            moveToPos(x, aPad);
            printChar(' ', farbaBiela);

            moveToPos(x, aPad + padSize);
            printChar(' ', farbaCierna);
        }
        else if (aMove < 0)
        {
            if (aPad + padSize > maxY - 1)
                return;

            moveToPos(x, aPad);
            printChar(' ', farbaCierna);

            moveToPos(x, aPad + padSize);
            printChar(' ', farbaBiela);

            ++aPad;
        }

        return;
    }

    void updateScore() {
        moveToPos(31, 0);
        printChar('S', farbaKontrast);
        printChar('c', farbaKontrast);
        printChar('o', farbaKontrast);
        printChar('r', farbaKontrast);
        printChar('e', farbaKontrast);
        printChar(' ', farbaBiela);
        printChar((iScoreLeft / 10) + '0', farbaKontrast);
        printChar(iScoreLeft + '0', farbaKontrast);
        printChar(' ', farbaBiela);
        printChar(':', farbaKontrast);
        printChar((iScoreRight / 10) + '0', farbaKontrast);
        printChar(iScoreRight + '0', farbaKontrast);
    }
public:
    pingPong(const char *aPort, char aPosX, char aPosY) : iCheck_buffer(), iSerial_out()
    {
        if(aPosX > maxX || aPosY > maxY) {
            throw std::invalid_argument("Neplatne pociatocne pozicie");
        }
        iPrevX = iScrPosX = aPosX;
        iPrevY = iScrPosY = aPosY;
        iRealX = aPosX * 100;
        iRealY = aPosY * 100;

        iPadLeft = 19;
        iPadRight = 19;

        iMoveX = 50;
        iMoveY = 50;

        char error = iSerial_out.openDevice(aPort, 115200);

        if (error != 1)
        {
            throw std::runtime_error("Nepodarilo sa pripojit k zbernici!");
        }

        iSerial_out.flushInQue();
    }

    ~pingPong()
    {
        iSerial_out.closeDevice();
    };

    void writePosChar(TPosChar & aOper) {
        
        moveToPos(aOper.iPosX, aOper.iPosY);

        printChar(aOper.iChar, aOper.iAtr);
    }

    void writePosChar(char aChar, char aAtr)
    {
        moveToPos(iScrPosX, iScrPosY);

        printChar(aChar, aAtr);
        // iCheck_buffer.push_back({iPosX, iPosY, aChar, aAtr});
    }

    // automatically writes blank white character
    void writePosChar()
    {
        moveToPos(iScrPosX, iScrPosY);

        printChar(' ', farbaBiela);
        // iCheck_buffer.push_back({iPosX, iPosY, ' ', farbaBiela});
    }

    // basically writes blank black character at current position
    void clearPosChar()
    {
        moveToPos(iScrPosX, iScrPosY);

        printChar(' ', farbaCierna);
        // iCheck_buffer.push_back({iPosX, iPosY, ' ', farbaCierna});
    }

    void moveLeftPadUp()
    {
        movePad(iPadLeft, +1);
        return;
    }

    void moveLeftPadDown()
    {
        movePad(iPadLeft, -1);
        return;
    }

    void moveRightPadUp()
    {
        movePad(iPadRight, +1);
        return;
    }

    void moveRightPadDown()
    {
        movePad(iPadRight, -1);
        return;
    }

    void initScreen() {
        moveToPos(0, 0);
        size_t i = 0, u = 0;
        for (; i < 80; ++i)
        {
            printChar(' ', farbaBiela);
        }

        for (u = 1; u < 39; ++u)
        {
            for (i = 0; i < 80; ++i)
            {
                printChar(' ', farbaCierna);
            }
        }

        moveToPos(0, 39);
        for (i = 0; i < 80; ++i)
        {
            printChar(' ', farbaBiela);
        }

        for (size_t i = 0; i < padSize; ++i)
        {
            moveToPos(0, iPadLeft + i);
            printChar(' ', farbaBiela);

            moveToPos(79, iPadRight + i);
            printChar(' ', farbaBiela);
        }

        updateScore();

        return;
    }

    // Starts the checking and writing process
    void flushBuffer()
    {
        char writeBuffer[4] = {};
        unsigned int bytesWritten = 4;

        if (iSerial_out.getInQueStat() != iCheckCount * 4)
        {
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

                do
                {
                    iSerial_out.writeBytes(writeBuffer, 4, &bytesWritten);
                    if (bytesWritten != 4)
                    {
                        continue;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    if (iSerial_out.getInQueStat() < 2)
                    {
                        iSerial_out.flushInQue();
                        continue;
                    }
                    iSerial_out.readBytes(readBuffer, 2, 0, 0);
                    iSerial_out.flushInQue();

                } while (readBuffer[0] != 'A' || readBuffer[1] != 'p');

                writeBuffer[0] = 'c';
                writeBuffer[1] = iter.iChar;
                writeBuffer[2] = iter.iAtr;

                do
                {
                    iSerial_out.writeBytes(writeBuffer, 4, &bytesWritten);
                    if (bytesWritten != 4)
                    {
                        continue;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    if (iSerial_out.getInQueStat() < 2)
                    {
                        iSerial_out.flushInQue();
                        continue;
                    }
                    iSerial_out.readBytes(readBuffer, 2, 0, 0);
                    iSerial_out.flushInQue();

                } while (readBuffer[0] != 'A' || readBuffer[1] != 'c');

                iCheck_buffer.pop_front();
            }
            iCheckCount = 0;
        }
        else
        {
            // If we received everything we can check if the data are valid

            for (size_t i = 0; i < iCheckCount; ++i)
            {
                if (iSerial_out.getInQueStat() < 4)
                {
                    throw std::runtime_error("Not expected!");
                }

                iSerial_out.readBytes(writeBuffer, 4, 0, 0);

                // error checking
                if (writeBuffer[0] != 'A' || writeBuffer[1] != 'p' || writeBuffer[2] != 'A' || writeBuffer[3] != 'c')
                {
                    // Invalid return value detected repeat the write
                    iCheck_buffer.push_back(iCheck_buffer.front());
                }
                iCheck_buffer.pop_front();
            }
            iCheckCount = 0;
        }

        writeBuffer[3] = 'l';
        for (auto &iter : iCheck_buffer)
        {
            writeBuffer[0] = 'p';
            writeBuffer[1] = iter.iPosX;
            writeBuffer[2] = iter.iPosY;

            iSerial_out.writeBytes(writeBuffer, 4, &bytesWritten);
            if (bytesWritten != 4)
            {
                break;
            }

            writeBuffer[0] = 'c';
            writeBuffer[1] = iter.iChar;
            writeBuffer[2] = iter.iAtr;

            iSerial_out.writeBytes(writeBuffer, 4, &bytesWritten);
            if (bytesWritten != 4)
            {
                break;
            }

            ++iCheckCount;
        }
    }

    // calculates new X and Y positions
    void newPos()
    {
        iRealX += iMoveX;
        iRealY += iMoveY;

        // has change occured?
        if(iRealX / 100 != iScrPosX || iRealY / 100 != iScrPosY) {
            iPrevX = iScrPosX;
            iPrevY = iScrPosY;

            iScrPosX = (char)(iRealX / 100);
            iScrPosY = (char)(iRealY / 100);

            // Odrazy od stien
            if(iScrPosY >= maxY - 1) {
                iMoveY = -abs(iMoveY);
                // TODO: osetrenie hranic
                iMoveY += (rand() % 3) - 1;
            }
            else if(iScrPosY <= minY + 1) {
                iMoveY = abs(iMoveY);
                // TODO: osetrenie hranic
                iMoveY += (rand() % 3) - 1;
            }

            // Right
            if(iScrPosX >= (maxX - 1) && iScrPosY >= iPadRight && iScrPosY < (iPadRight + padSize)) {
                iMoveX = -abs(iMoveX) - 5;
                char change = getKeyDownRight();
                if(change != 0) {
                    iMoveY += 10 * change;
                }
            }
            else if(iScrPosX >= maxX) {
                // Výhra!!! 
                iMoveX = -50;
                iMoveY = (rand() % 100) - 50;
                iRealX = 39 * 100;
                iRealY = 20 * 100;
                iScrPosX = 39;
                iScrPosY = 20;

                ++iScoreLeft;
                updateScore();

                return;
            }
            // Left
            else if(iScrPosX <= (minX + 1) && iScrPosY >= iPadLeft && iScrPosY < (iPadLeft + padSize)) {
                iMoveX = abs(iMoveX) + 5;
                char change = getKeyDownLeft();
                if(change != 0) {
                    iMoveY += 10 * change;
                }
            }
            else if(iScrPosX <= minX) {
                // Výhra!!! 
                iMoveX = 50;
                iMoveY = (rand() % 100) - 50;
                iRealX = 40 * 100;
                iRealY = 20 * 100;
                iScrPosX = 40;
                iScrPosY = 20;

                ++iScoreRight;
                updateScore();

                return;
            }
        }

        return;
    }
};

int main(int argc, char const *argv[])
{
    char error;
    try {
        pingPong game(SERIAL_PORT_OUT, 40, 20);

        game.initScreen();

        while(!(GetKeyState(VK_ESCAPE) & WM_KEYDOWN)) {
            // Clear previous position
            game.clearPosChar();

            game.newPos();

            // Write new position
            game.writePosChar();

            switch (getKeyDownLeft())
            {
                case -1:
                    game.moveLeftPadUp();
                    break;
                case 1:
                    game.moveLeftPadDown();
                default:
                    break;
            }

            switch (getKeyDownRight())
            {
                case -1:
                    game.moveRightPadUp();
                    break;
                case 1:
                    game.moveRightPadDown();
                default:
                    break;
            }
            
            Sleep(20);
        }
    }
    catch (...) {
        std::cerr << "an exception was thrown" << std::endl;
    }

    //std::cin >> error;
    return 0;
}

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

