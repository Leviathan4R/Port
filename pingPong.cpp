#include <assert.h>
#include <stdio.h>
#include <deque>
#include <exception>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include "serialib.h"

#define SERIAL_PORT_OUT "\\\\?\\COM10"

/*
X: 30-40 death zone
*/

constexpr char farbaBiela = (char)0b01111111;
constexpr char farbaKontrast = (char)0b00000111;
constexpr char farbaCierna = 0b00000000;
constexpr char padSize = 6;
constexpr char minX = 0, maxX = 79, minY = 0, maxY = 39;
constexpr int posResX = 100 * 80 - 1, posResY = 100 * 40 - 1;

struct TPosChar {
    char iPosX;
    char iPosY;
    char iAtr;
    char iChar;
};

int clrScreen(serialib &aSerial) {
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
    std::deque<unsigned char> iWriteBuffer;
    std::deque<unsigned char> iCheckBuffer;
    serialib iSerialOut;

    char iScoreLeft = 0, iScoreRight = 0;

    char iPadLeft = 19, iPadRight = 19;
    char iScrPosX, iScrPosY, iPrevX, iPrevY;
    int iRealX, iRealY;
    int iMoveX, iMoveY;

    void moveToPos(char const &aX, char const &aY) {
        iWriteBuffer.push_back(aX);
        iWriteBuffer.push_back(aY);
        return;
    }

    void printChar(char const &aCharacter, char const &aParams) {
        
        iWriteBuffer.push_back(aCharacter | 0x80);
        iWriteBuffer.push_back(aParams | 0x80);
        return;
    }

    void printString(const char* aString, char const& aParam) {
        if(!aString) {
            return;
        }
        while(*aString) {
            iWriteBuffer.push_back(*aString | 0x80);
            iWriteBuffer.push_back(aParam | 0x80);
            aString = aString + 1;
        }
        return;
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
        printString("Score ", farbaKontrast);
        printChar((iScoreLeft / 10) + '0', farbaKontrast);
        printChar(iScoreLeft + '0', farbaKontrast);
        printChar(' ', farbaBiela);
        printChar(':', farbaKontrast);
        printChar((iScoreRight / 10) + '0', farbaKontrast);
        printChar(iScoreRight + '0', farbaKontrast);
    }
public:
    pingPong() = default;

    pingPong(char aPosX, char aPosY) : iWriteBuffer(), iSerialOut()
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
    }

    ~pingPong() {
        iSerialOut.closeDevice();
    }

    void connect(const char *aPort) {
        char error = iSerialOut.openDevice(aPort, 115200);

        if (error != 1) {
            iSerialOut.closeDevice();
            throw std::runtime_error("Nepodarilo sa pripojit k zbernici!");
        }

        iWriteBuffer.clear();
        iCheckBuffer.clear();

        iSerialOut.flushInQue();
    }

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

    void readKeyboard() {
        switch (getKeyDownLeft())
        {
            case -1:
                movePad(iPadLeft, +1);
                break;
            case 1:
                movePad(iPadLeft, -1);
            default:
                break;
        }

        switch (getKeyDownRight())
        {
            case -1:
                movePad(iPadRight, +1);
                break;
            case 1:
                movePad(iPadRight, -1);
            default:
                break;
        }
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

    bool checkConnection() {
        return iSerialOut.isDeviceOpen();
    }

    // Starts the checking and writing process
    void flushBuffer() {
        unsigned char writeBuffer[4] = {'c', ' ', farbaBiela, 'l'};
        unsigned int bytesWritten = 4;

        writeBuffer[3] = 'l';
        for (size_t i = 0; (i+1) < iWriteBuffer.size(); i += 2)
        {
            writeBuffer[0] = 'p';
            writeBuffer[1] = iWriteBuffer[i];
            writeBuffer[2] = iWriteBuffer[i+1];

            if(writeBuffer[1] & 0x80) {
                writeBuffer[0] = 'c';
                writeBuffer[1] &= 0x7F;
                writeBuffer[2] &= 0x7F;
            }

            iSerialOut.writeBytes(writeBuffer, 4, &bytesWritten);
            if (bytesWritten != 4)
            {
                if(iSerialOut.getInQueStat() > 4000) {
                    i -= 2;
                    continue;
                }
                iSerialOut.closeDevice();
                throw std::runtime_error("Kábel bol odpojený!");
            }
            iCheckBuffer.push_back(writeBuffer[0]);

            if(iSerialOut.getInQueStat() >= 2) {
                if(checkResponse() < 0) {
                    // NOP
                }
            }
        }

        while (iWriteBuffer.size()) {
            iWriteBuffer.pop_front();
        }
    }

    int checkResponse() {
        unsigned char readBuffer[2] = {};
        int count = 0;

        while(iSerialOut.getInQueStat() >= 2) {
            iSerialOut.readBytes(readBuffer, 2, 0, 0);

            if (readBuffer[0] != 'A' || readBuffer[1] != iCheckBuffer.front())
            {
                std::cout << "loss detected!" << std::endl;
                return -1;
            }

            iCheckBuffer.pop_front();
            ++count;
        }

        return count;
    }

    // calculates new X and Y positions
    void newPos()
    {
        int newX = iRealX + iMoveX;
        int newY = iRealY + iMoveY;

        // Update 
        iRealX += iMoveX;
        iRealY += iMoveY;

        // has change occured?
        if(newX / 100 != iScrPosX || newY / 100 != iScrPosY) {

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
            if(iScrPosX == (maxX - 1) && iScrPosY >= iPadRight && iScrPosY < (iPadRight + padSize)) {
                iMoveX = -(abs(iMoveX) + 5);
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
                iScrPosX = (char)(iRealX / 100);
                iScrPosY = (char)(iRealY / 100);

                ++iScoreLeft;
                updateScore();
            }

            // Left
            else if(iScrPosX == (minX + 1) && iScrPosY >= iPadLeft && iScrPosY < (iPadLeft + padSize)) {
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
                iScrPosX = (char)(iRealX / 100);
                iScrPosY = (char)(iRealY / 100);

                ++iScoreRight;
                updateScore();
            }
        }

        return;
    }
};
 
std::chrono::steady_clock::time_point now() { return std::chrono::steady_clock::now(); }
 
std::chrono::steady_clock::time_point awake_time()
{
    std::chrono::milliseconds time(20);
    return now() + time;
}

int main(int argc, char const *argv[])
{
    char error;
    pingPong game(40, 20);
    std::cout << "Pro ukončení stlač ESC!" << std::endl;

    while(!(GetKeyState(VK_ESCAPE) & WM_KEYDOWN)) {
        try {
            if(!game.checkConnection()) {
                std::cout << "Hra se načíta.... " << std::endl;
                game.connect(SERIAL_PORT_OUT);

                game.initScreen();

                game.flushBuffer();
                std::cout << "Hra běží!" << std::endl;
            }
        
            auto start = now();
            auto wait = awake_time();
            // Clear previous position
            game.clearPosChar();

            // detects the keyboard inputs and moves the pads
            game.readKeyboard();

            // calculates the new position of ball
            game.newPos();

            // Write new position
            game.writePosChar();
            
            // Send to comm.
            game.flushBuffer();
            // Wait for response (also for synch. fps)
            std::this_thread::sleep_until(wait);
            game.checkResponse();
            
            std::chrono::duration<double, std::milli> elapsed{now() - start};
            //std::cout << "Waited " << elapsed.count() << " ms\n" << std::endl;    
        }
        catch (std::runtime_error & e) {
            std::cout << e.what() << "\nPripoj naspět kábel, a stlač ENTER. Pro ukončení stlač ESC!" << std::endl;
            while(true) {
                if(GetKeyState(VK_ESCAPE) & WM_KEYDOWN) {
                    return -1;
                }
                else if (GetKeyState(VK_RETURN) & WM_KEYDOWN) {
                    break;
                }

                Sleep(10);
            }
        }
    }

    //std::cin >> error;
    return 0;
}
