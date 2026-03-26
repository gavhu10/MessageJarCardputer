#include "event.h"
#include "messagejar.h"
#include <string>
#include <thread>

#define TICKS 1000

using std::string;

shared_ptr<vector<Message>> get(string room, std::mutex &userMutex,
                                MessageJar *user, size_t latest = 0) {
  std::lock_guard<std::mutex> lock(userMutex);
  return user->get_messages(room, latest);
}

void messageTask(void *pvParameters) {
  // Cast the void pointer back to our struct
  MessageTaskParams *params = (MessageTaskParams *)pvParameters;

  size_t latest_message = 0;

  unsigned long last_executed = millis();
  unsigned long current_time =
      TICKS + 1 + last_executed; // get messages right away

  while (*(params->running)) {
    if ((current_time - last_executed) > TICKS) {

      shared_ptr<vector<Message>> messages =
          get(params->room, *(params->userMutex), params->user, latest_message);

      if (messages && messages->size()) {
        latest_message += messages->size();
        string buffer = "";
        // buffer += std::to_string(messages->size());
        for (const auto &msg : *messages) {
          buffer += msg.as_string();
        }

        std::lock_guard<std::mutex> lock(*(params->receiveMutex));

        *(params->receiveString) = buffer;
        *(params->sendDataFlag) = true;
      }

      last_executed = millis();
    }
    current_time = millis();
  }

  delete params;
  vTaskDelete(NULL);
}