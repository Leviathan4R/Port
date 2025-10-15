

#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

#include "serialib.h"
#include <chrono>
#include <thread>
//#include <windows.h>


#define SERIAL_PORT_OUT "COM4"
int readyToRead = 0;

const size_t bufSize = 55;
static bool run = true;
serialib serial_out;


const char buffer[bufSize] = "Ahoj svet!, Hello World!, Hallo Welt!, Ahoj Světe!";
char readBuffer[bufSize] = {0};

int writeLoop() {
    while(run) {
        printf("%i,", serial_out.available());

        while (serial_out.available() > bufSize) {
            serial_out.readBytes(readBuffer, bufSize, 0, 0);
/*
            for (size_t i = 0; i < bufSize; ++i)
            {
                if (buffer[i] != readBuffer[i]) {
                    run = false;
                }
            }*/
        }
        //serial_out.flushReceiver();
        
        serial_out.writeString(buffer);
    }
}

int main() {

    // serialib serial_out;
    // serialib serial_in;

    
    char errorOpening = serial_out.openDevice(SERIAL_PORT_OUT, 9600);
    if (errorOpening != 1) {
        return -1;
    }
    serial_out.closeDevice();

    errorOpening = serial_out.openDevice(SERIAL_PORT_OUT, 115200);
    
    if (errorOpening != 1) {
        return -1;
    }

    

    std::cout << "Buffer: "<< buffer << std::endl;

    writeLoop();

    /*
    for (size_t i = 0; i < bufSize; ++i)
    {
        serial_out.writeChar(buffer[i]);

        serial_out.readBytes(&readChar, 1, 1);

        readBuffer[i] = readChar;

        std::cout << readChar << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(4));
    }*/

    std::cout<< "Original buffer: " << buffer <<std::endl;
    std::cout<< "Read buffer: " << readBuffer <<std::endl;

    serial_out.closeDevice();

    std::cout << "Program Konci" << std::endl;
    std::cin >> readBuffer;
    
    return 0;
}