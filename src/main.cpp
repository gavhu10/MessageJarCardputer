// current state: I can't receive messages. Sending works fine.
#include <M5Cardputer.h>
#include <thread>
#include <atomic>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "input.h"
#include "display.h"
#include "serial.h"
#include "event.h"
#include "messagejar.h"
#include "SdService.h"

#include <string>

using std::make_shared;
using std::shared_ptr;
using std::string;

// Config
#define CONFIG_FILE_PATH "/mjconfig.json"
BaudRate baudRate = BAUD_9600;
uint8_t rxPin = 1;
uint8_t txPin = 2;
uint8_t dataBits = 8;
ParityType parity = NONE;
uint8_t stopBits = 1;
bool flowControl = false;
bool inverted = false;
uint8_t selectedIndex = LAUNCH_INDEX;
short times_before_refresh = 5;

string SSID = "";
string PASSWORD = "";
string ROOM = "";
string USERNAME = "";
string USERPASSWORD = "";

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
    USERPASSWORD = data.at("userpassword");
    USERNAME = data.at("username");
    ROOM = data.at("room");
  }
  catch (const std::out_of_range &)
  {
    // SSID = "";
    // PASSWORD = "";
    // USERPASSWORD = "";
    // USERNAME = "";
    // ROOM = "";
    displayMessageBox("Config file is missing required fields!");
    while (true)
    {
      delay(1000);
    }
  }
}

void _config()
{
  bool firstRender = true;
  while (true)
  {
    char input = configInputHandler();
    selectedIndex = handleIndexSelection(input, selectedIndex);
    handleConfigSelection(input, baudRate, rxPin, txPin, dataBits, parity, stopBits, flowControl, inverted, selectedIndex);

    if (input != KEY_NONE || firstRender)
    {
      displayConfig(baudRateToInt(baudRate), rxPin, txPin, dataBits, parityToString(parity), stopBits, flowControl, inverted, selectedIndex);
      firstRender = false;
    }

    // If user presses the start button, we leave config screen
    if (input == KEY_OK && selectedIndex == LAUNCH_INDEX)
    {
      displayClearMainView();
      running = true;
      break;
    }
  }
}

void terminal()
{
  int16_t promptSize = -1;
  // int16_t terminalSize = -1;

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
      displayTerminal(receiveString);
      receiveString.clear();
      receiveDataFlag = false;
    }

    if (promptSize != sendString.size())
    {
      displayPrompt(sendString);
      promptSize = sendString.size();
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

  // WiFi connect
  User = new MessageJar(USERNAME, USERPASSWORD);

  WiFi.begin(SSID.c_str(), PASSWORD.c_str());

  displayMessageBox("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  displayMessageBox("Connected to WiFi!");
  assert(User->check());

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
}

void loop()
{
  terminal();
}