#include "event.h"
#include "messagejar.h"
#include <string>
#include <thread>

using std::string;

shared_ptr<vector<Message>> get(string room, std::mutex &userMutex, MessageJar *user, size_t latest = 0)
{
    std::lock_guard<std::mutex> lock(userMutex);
    return user->get_messages(room, latest);
}

void messageTask(void *pvParameters)
{
    // Cast the void pointer back to our struct
    MessageTaskParams *params = (MessageTaskParams *)pvParameters;

    size_t latest_message = 0;

    while (*(params->running))
    {

        shared_ptr<vector<Message>> messages = get(params->room, *(params->userMutex), params->user, latest_message);

        if (messages && messages->size())
        {
            latest_message += messages->size();
            string buffer = "";
            buffer += messages->size();
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