#include <M5Cardputer.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "input.h"
#include "display.h"
#include "event.h"
#include "messagejar.h"
#include "SdService.h"

#include <string>
#include <mutex>
#include <thread>
#include <atomic>

using std::make_shared;
using std::shared_ptr;
using std::string;

// Config
#define CONFIG_FILE_PATH "/mjconfig.json"
bool flowControl = false;
bool inverted = false;
short times_before_refresh = 5;

string SSID = "";
string PASSWORD = "";
string ROOM = "";
string TOKEN = "";

// Var atomic for receive thread
std::atomic<bool> receiveDataFlag(false);
std::atomic<bool> running(true);

// Strings for serial send/receive
std::string sendString;
std::string receiveString;

// Lock receiveString and User for thread safe purpose
std::mutex receiveMutex;
std::mutex userMutex;

// MessageJar instance
MessageJar *User = nullptr;

// SdService instance
SdService SDCard;

void send(string message)
{
  std::lock_guard<std::mutex> lock(userMutex);
  User->send(std::string{ROOM}, std::string{message});
}

void config()
{

  std::map<string, string> data = {};

  if (!SDCard.isFile(CONFIG_FILE_PATH))
  {
    displayMessageBox("Config file not found!");
    while (true)
    {
      delay(1000);
    }
  }
  auto configData = SDCard.readFile(CONFIG_FILE_PATH);

  JsonDocument doc;
  deserializeJson(doc, configData.c_str());
  JsonObject obj = doc.as<JsonObject>();

  for (auto pair : obj)
  {
    data[pair.key().c_str()] = pair.value().as<string>();
  }

  try
  {
    SSID = data.at("ssid");
    PASSWORD = data.at("wifipassword");
    TOKEN = data.at("token");
  }
  catch (const std::out_of_range &)
  {
    displayMessageBox("Config is malformed!");
    while (true)
    {
      delay(1000);
    }
  }

  User = new MessageJar(TOKEN);

  // WiFi connect
  WiFi.begin(SSID.c_str(), PASSWORD.c_str());

  displayMessageBox("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  displayMessageBox("Connected to WiFi!");
  if (!User->check())
  {
    displayMessageBox("User auth failed!");
    while (true)
    {
      delay(1000);
    }
  }
  displayMessageBox("Getting rooms...");

  auto rooms = User->get_rooms();
  if (!rooms)
  {
    displayMessageBox("No rooms found!");
    while (true)
    {
      delay(1000);
    }
  }
  else
  {
    rooms->push_back("+ Create new room");
    int num = selectFromList(*rooms);
    if (num == rooms->size() - 1)
    { // then we are creating a new room
      ROOM = getInput("");
      if (!User->create_room(ROOM))
      {
        displayMessageBox("Failed to make room");
        for (;;)
          delay(1000);
      }
    }
    else
    {
      ROOM = rooms->at(num);
    }
  }
}

void terminal()
{
  int16_t promptSize = -1;
  // int16_t terminalSize = -1;
  size_t scroll = 0;
  bool redraw = false;
  string messages = "";

  while (running)
  {
    {
      char input = promptInputHandler();

      switch (input)
      {
      case KEY_NONE:
        break;
      case KEY_OK:
        send(sendString);
        sendString.clear();
        break;
      case KEY_DEL:
      {
        if (!sendString.empty())
        {
          sendString.pop_back();
        }
      }
      break;
      case KEY_ARROW_DOWN:
      {
        if (scroll > 0)
        {
          --scroll;
          redraw = true;
        }
        break;
      }
      case KEY_ARROW_UP:
      {
        ++scroll;
        redraw = true;
        break;
      }
      default:
      {
        sendString += input;
      }
      break;
      }
    }

    if (receiveDataFlag) // if data has been be recived (receiveDataFlag)
    {
      std::lock_guard<std::mutex> lock(receiveMutex);
      messages += receiveString;
      receiveString.clear();
      redraw = true;
      receiveDataFlag = false;
    }

    if (promptSize != sendString.size())
    {
      displayPrompt(sendString);
      promptSize = sendString.size();
    }

    if (redraw)
    {
      displayTerminal(messages, scroll);
      redraw = false;
    }

    delay(100);
  }
}

void setup()
{
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);

  displayInit();

  SDCard.begin();

  // displayWelcome();
  // delay(2000);

  // config
  config();

  MessageTaskParams *params = new MessageTaskParams{
      &receiveDataFlag,
      &receiveString,
      &running,
      &receiveMutex,
      &userMutex,
      User,
      ROOM,
  };

  xTaskCreate(     // Using xTaskCreate to manage memory better
      messageTask, // Function to run
      "MsgTask",   // Name (for debugging)
      8192,        // Stack size (in bytes)
      params,      // Parameter to pass
      1,           // Priority
      NULL         // Task handle
  );

  displayClearMainView();
  displayMessageBox("Loading messages...");
}

void loop()
{
  terminal();
}