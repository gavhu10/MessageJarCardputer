#include "event.h"
#include "messagejar.h"
#include <string>
#include <thread>

using std::string;

shared_ptr<vector<Message>> get(string room, std::mutex &userMutex, MessageJar *user)
{
    std::lock_guard<std::mutex> lock(userMutex);
    return user->get_messages(room);
}

uint8_t handleIndexSelection(char input, uint8_t currentIndex)
{
    switch (input)
    {
    case KEY_ARROW_UP:
        if (currentIndex == LAUNCH_INDEX)
            return BITS_INDEX;
        if (currentIndex == PARITY_INDEX)
            return LAUNCH_INDEX;
        return currentIndex > 0 ? currentIndex - 1 : LAUNCH_INDEX;
    case KEY_ARROW_DOWN:
        if (currentIndex == BITS_INDEX)
            return LAUNCH_INDEX;
        return (currentIndex < LAUNCH_INDEX) ? currentIndex + 1 : 0;
    case KEY_ARROW_LEFT:
        return currentIndex >= 4 ? currentIndex - 4 : currentIndex;
    case KEY_ARROW_RIGHT:
        return currentIndex < 4 ? currentIndex + 4 : currentIndex;
    default:
        return currentIndex;
    }
}

void handleConfigSelection(char input, BaudRate &baudRate, uint8_t &rxPin, uint8_t &txPin,
                           uint8_t &dataBits, ParityType &parity, uint8_t &stopBits,
                           bool &flowControl, bool &inverted, uint8_t selectedIndex)
{
    if (input != KEY_OK)
        return;

    switch (selectedIndex)
    {
    case RXPIN_INDEX:
    case TXPIN_INDEX:
        rxPin = (rxPin == 1) ? 2 : 1;
        txPin = (txPin == 1) ? 2 : 1;
        break;
    case BAUD_INDEX:
        baudRate = static_cast<BaudRate>((baudRate == BAUDRATE_COUNT - 1) ? 0 : baudRate + 1);
        break;
    case BITS_INDEX:
        dataBits = (dataBits < 8) ? dataBits + 1 : 5;
        break;
    case PARITY_INDEX:
        parity = static_cast<ParityType>((parity == PARITY_COUNT - 1) ? 0 : parity + 1);
        break;
    case STOP_BITS_INDEX:
        stopBits = (stopBits == 1) ? 2 : 1;
        break;
    case FLOW_CONTROL_INDEX:
        flowControl = !flowControl;
        break;
    case INVERTED_INDEX:
        inverted = !inverted;
        break;
    default:
        break;
    }
}

void messageTask(void *pvParameters)
{
    // Cast the void pointer back to our struct
    MessageTaskParams *params = (MessageTaskParams *)pvParameters;

    while (*(params->running))
    {

        shared_ptr<vector<Message>> messages = get(params->room, *(params->userMutex), params->user);

        if (messages)
        {
            string buffer = "";
            for (const auto &msg : *messages)
            {
                buffer += msg.as_string();
            }

            std::lock_guard<std::mutex> lock(*(params->receiveMutex));
            *(params->receiveString) = buffer;
            *(params->sendDataFlag) = true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    delete params;
    vTaskDelete(NULL);
}