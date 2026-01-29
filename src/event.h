#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <atomic>
#include <mutex>
#include "input.h"
#include "messagejar.h"

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

shared_ptr<vector<Message>> get(string room, std::mutex &userMutex, MessageJar *user);

void messageTask(void *pvParameters);

#endif