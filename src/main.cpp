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

void logout()
{
  auto configData = SDCard.readFile(CONFIG_FILE_PATH);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configData);

  doc["token"] = "";

  string output;
  serializeJson(doc, output);

  SDCard.writeFile(CONFIG_FILE_PATH, output.c_str());
  if (confirm("Do you want to revoke  this token?"))
  {
    User->revoke();
  }
  showMessage("Rebooting...");
  delay(100);
  ESP.restart();
}

void send(string message, string room)
{
  std::lock_guard<std::mutex> lock(userMutex);
  User->send(room, message);
}

std::pair<string, string> connect_to_wifi(std::map<string, string> config)
{

  vector<string> foundSSIDs;
  string SSID = "";

  int n = WiFi.scanNetworks();

  if (n > 0)
  {
    for (int i = 0; i < n; ++i)
    {
      // Add each SSID to the vector
      foundSSIDs.push_back(WiFi.SSID(i).c_str());
    }
  }

  foundSSIDs.push_back("Enter SSID...");
  int ssidIndex = selectFromList(foundSSIDs);
  if (ssidIndex == foundSSIDs.size() - 1)
  {
    SSID = getInput("SSID");
  }
  else
  {
    SSID = foundSSIDs[ssidIndex];
  }

  string password = "";

  if (config.find(SSID) == config.end())
  {
    password = getInput("Password");
  }
  else
  {
    password = config[SSID];
  }

  WiFi.begin(SSID.c_str(), password.c_str());

  showMessage("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  // return the wifi password if it was not in the config

  if (config.find(SSID) == config.end())
  {
    return {SSID, password};
  }

  return {"", ""};
}

void connect_user(JsonDocument &doc, MessageJar *&user)
{

  string token = doc["token"].as<string>();
  user = new MessageJar(token);

  if (token.empty() || !user->check())
  {
    string username = getInput("Username");
    string password = "";
    string token = "";

    if (MessageJar::user_exists(username))
    {
      password = getInput("Log in");
    }
    else
    {
      password = getInput("Create account");
      string password2 = getInput("Confirm password");
      if (password != password2)
      {
        showMessage("Passwords don't match!");
        while (true)
        {
          delay(1000);
        }
      }
      if (!MessageJar::create_user(username, password))
      {
        showMessage("User creation failed!");
        while (true)
        {
          delay(1000);
        }
      }
    }

    int num = (esp_random() % 900) + 100; // number between 100 and 999
    string name = "Cardputer-" + std::to_string(num);
    token = MessageJar::generate_token(username, password, name);
    if (token.empty())
    {
      showMessage("Log in fail");
      while (true)
      {
        delay(1000);
      }
    }
    doc["token"] = token;
    string output;
    serializeJson(doc, output);
    SDCard.writeFile(CONFIG_FILE_PATH, output.c_str());
    delete user;
    user = new MessageJar(token);
  }
}

void config()
{
  auto configData = SDCard.readFile(CONFIG_FILE_PATH);

  JsonDocument doc;
  JsonDocument WIFI_CREDS;
  DeserializationError error = deserializeJson(doc, configData);
  std::map<string, string> wifiMap;

  if (error)
  {
    showMessage("Malformed config!");
    while (true)
    {
      delay(1000);
    }
  }

  WIFI_CREDS = doc["wifi"];

  for (auto pair : WIFI_CREDS.as<JsonObject>())
  {
    wifiMap[pair.key().c_str()] = pair.value().as<string>();
  }

  showMessage("Getting SSIDs...");

  std::pair<string, string> creds = connect_to_wifi(wifiMap);

  if (!creds.first.empty())
  {
    // Update the JSON document with the new credentials
    doc["wifi"][creds.first] = creds.second;

    string output;
    serializeJson(doc, output);

    SDCard.writeFile(CONFIG_FILE_PATH, output.c_str());
  }

  showMessage("WiFi connected!");

  connect_user(doc, User);

  if (!User->check())
  {
    showMessage("User auth failed!");
    while (true)
    {
      delay(1000);
    }
  }
}

string get_room()
{
  string ret;

  showMessage("Getting rooms...");

  auto rooms = User->get_rooms();
  if (!rooms)
  {
    showMessage("Error getting rooms!");
    while (true)
    {
      delay(1000);
    }
  }
  else
  {
    rooms->push_back("+ Create new room");
    rooms->push_back("+ Logout...");
    int num = selectFromList(*rooms);
    if (num == rooms->size() - 2)
    { // then we are creating a new room
      ret = getInput("Room name");
      if (!User->create_room(ret))
      {
        showMessage("Failed to make room");
        for (;;)
          delay(1000);
      }
    }
    else if (num == rooms->size() - 1)
    {
      logout();
    }
    else
    {
      ret = rooms->at(num);
    }
  }
  return ret;
}

void terminal(string room)
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
        send(sendString, room);
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
      case KEY_ESC:
      {
        running = false;
        displayClearMainView();
        showMessage("Exiting...");
        break;
      }
      default:
      {
        sendString += input;
      }
      break;
      }
    }

    if (receiveDataFlag.exchange(false)) // if data has been be recived (receiveDataFlag)
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

  if (!SDCard.begin())
  {
    {
      showMessage("No SD card!");
      while (true)
      {
        delay(1000);
      }
    }
  }

  // displayWelcome();
  // delay(2000);

  // config
  config();
}

void loop()
{

  string room = get_room();
  running = true;

  MessageTaskParams *params = new MessageTaskParams{
      &receiveDataFlag,
      &receiveString,
      &running,
      &receiveMutex,
      &userMutex,
      User,
      room,
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
  showMessage("Loading messages...");
  terminal(room);
  // if we are here the user pressed esc
}