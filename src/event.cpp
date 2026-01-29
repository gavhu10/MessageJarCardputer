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