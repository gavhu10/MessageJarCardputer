#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <atomic>
#include <mutex>
#include "serial.h"
#include "input.h"
#include "messagejar.h"

enum ConfigIndex
{
    RXPIN_INDEX = 0,
    TXPIN_INDEX,
    BAUD_INDEX,
    BITS_INDEX,
    PARITY_INDEX,
    STOP_BITS_INDEX,
    FLOW_CONTROL_INDEX,
    INVERTED_INDEX,
    LAUNCH_INDEX,
};

struct MessageTaskParams
{
    std::atomic<bool> *sendDataFlag;
    string *receiveString;
    std::atomic<bool> *running;
    std::mutex *receiveMutex;
    std::mutex *userMutex;
    MessageJar *user;
    string room;
};

uint8_t handleIndexSelection(char input, uint8_t currentIndex);

void handleConfigSelection(char input, BaudRate &baudRate, uint8_t &rxPin, uint8_t &txPin,
                           uint8_t &dataBits, ParityType &parity, uint8_t &stopBits,
                           bool &flowControl, bool &inverted, uint8_t selectedIndex);

void messageTask(void *pvParameters);

#endif