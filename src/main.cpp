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

// Var atomic for input thread
std::atomic<bool> sendDataFlag(false);
std::atomic<bool> running(true);

// Strings for serial send/receive
std::string sendString;
std::string receiveString;

// Lock sendString for thread safe purpose
std::mutex sendMutex;

// MessageJar instance
MessageJar *User = nullptr;

// SdService instance
SdService SDCard;

void config()
{

  std::map<string, string> data = {};

  if (!SDCard.isFile(CONFIG_FILE_PATH))
  {
    assert(0);
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
    SSID = "";
    PASSWORD = "";
    USERPASSWORD = "";
    USERNAME = "";
    ROOM = "";
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
  int16_t terminalSize = -1;

  short times = 0;

  while (running)
  {

    times++;

    times %= times_before_refresh;

    if (!times)
    {
      std::shared_ptr<std::vector<Message>> messages = User->get_messages(ROOM);
      string buffer = "";

      if (messages && messages->size())
      {
        for (const auto &msg : *messages)
        {
          buffer += msg.as_string();
        }

        receiveString = buffer;
      }
    }

    if (sendDataFlag)
    {
      // this is where output goes to
      User->send(std::string{ROOM}, std::string{sendString.c_str()});
      sendDataFlag = false;
      std::lock_guard<std::mutex> lock(sendMutex);
      sendString.clear();
    }

    if (terminalSize != receiveString.size())
    {
      displayTerminal(receiveString);
      terminalSize = receiveString.size();
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

  displayWelcome();
  delay(2000);

  // config
  config();


  // WiFi connect

  User = new MessageJar(USERNAME, USERPASSWORD);

  WiFi.begin(SSID.c_str(), PASSWORD.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  assert(User->check());

  // Prompt thread
  std::thread inputThread(handlePrompt, std::ref(sendDataFlag), std::ref(sendString),
                          std::ref(running), std::ref(sendMutex));
  inputThread.detach();
}

void loop()
{
  terminal();
}