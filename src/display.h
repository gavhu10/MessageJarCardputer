#ifndef DISPLAY_H
#define DISPLAY_H

#include <vector>
#include <string>
#include <M5Cardputer.h> // Assurez-vous que cette bibliothèque est la bonne selon votre matériel (M5Stack, M5Cardputer, etc.)

#define BACKGROUND_COLOR TFT_BLACK
#define PRIMARY_COLOR 0xfa03
#define RECT_COLOR_DARK 0x0841
#define RECT_COLOR_LIGHT 0xd69a
#define TEXT_COLOR 0xef7d
#define TEXT_COLOR_ALT TFT_DARKGRAY

#define DEFAULT_MARGIN 5
#define DEFAULT_ROUND_RECT 5

void displayInit();
void displayWelcome();
void displayStart(bool selected);
void displayTerminal(std::string terminalSting, size_t scroll = 0);
void displayPrompt(std::string sendString);
void displayClearMainView(uint8_t offsetY = 0);
void displayClearTerminalView();
void displayMessageBox(std::string message);
unsigned int selectFromList(std::vector<std::string> items, unsigned int startIndex = 0);
std::string getInput(std::string);

// Utility Function (forward declaration if needed)
void drawRect(bool selected, uint8_t margin, uint16_t startY, uint16_t sizeX, uint16_t sizeY);

#endif // DISPLAY_CONFIG_H
