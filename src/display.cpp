#include "display.h"
#include "input.h"

#include <memory>
#include <vector>

void displayInit()
{
    // Initialize display
    M5.Lcd.begin();
    M5.Lcd.setRotation(1);
    M5.Lcd.setTextColor(TFT_LIGHTGREY);
}

void displayWelcome()
{
    M5.Lcd.fillScreen(BACKGROUND_COLOR);

    // Main title
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(PRIMARY_COLOR);
    M5.Lcd.setCursor(50, 50);
    M5.Lcd.printf("Micro");

    M5.Lcd.setTextSize(2.5);
    M5.Lcd.setTextColor(TEXT_COLOR);
    M5.Lcd.setCursor(146, 54);
    M5.Lcd.printf("COM");

    // Sub title
    M5.Lcd.setTextSize(1.2);
    M5.Lcd.setTextColor(TEXT_COLOR);
    M5.Lcd.setCursor(50, 78);
    M5.Lcd.printf("Serial");

    M5.Lcd.setTextSize(1.3);
    M5.Lcd.setCursor(98, 78);
    M5.Lcd.printf("Communication");
};

void displayStart(bool selected)
{
    drawRect(selected, DEFAULT_MARGIN, 110, M5.Lcd.width() - 15, 25);
    M5.Lcd.setCursor(79, 118);
    M5.Lcd.print("START SERIAL");
};

void displayTerminal(std::string receiveString)
{
    const uint8_t charsPerLine = 39;
    const uint8_t linesPerScreen = 12;

    // Split receiveString by \n and wrap text
    std::vector<std::string> lines;
    std::string currentLine;
    for (size_t i = 0; i < receiveString.length(); ++i)
    {
        char c = receiveString[i];
        if (c == '\n')
        {
            lines.push_back(currentLine); // Add the current line and reset
            currentLine.clear();
        }
        else
        {
            currentLine += c;
            // If the current line exceeds the allowed number of characters per line, wrap it
            if (currentLine.length() >= charsPerLine)
            {
                lines.push_back(currentLine);
                currentLine.clear();
            }
        }
    }
    // Push the last remaining part of the string (if any)
    if (!currentLine.empty())
    {
        lines.push_back(currentLine);
    }

    // Now, calculate the number of lines and only display the last ones that fit on the screen
    size_t totalLines = lines.size();
    size_t startLine = 0;
    if (totalLines > linesPerScreen)
    {
        startLine = totalLines - linesPerScreen;
    }

    // Clear the terminal view before displaying the new content
    displayClearTerminalView();
    M5.Lcd.setCursor(0, DEFAULT_MARGIN);
    M5.Lcd.setTextSize(1);

    // Print only the visible portion of the terminal string
    for (size_t i = startLine; i < totalLines; ++i)
    {
        M5.Lcd.println(lines[i].c_str());
    }
}

void displayPrompt(std::string sendString)
{

    if (sendString.length() > 26)
    {
        sendString = sendString.substr(sendString.length() - 26);
    }
    drawRect(false, DEFAULT_MARGIN, 110, M5.Lcd.width() - 15, 25);
    M5.Lcd.setCursor(DEFAULT_MARGIN * 2, 118);
    M5.Lcd.print(" > ");
    M5.Lcd.print(sendString.c_str());
}

void drawRect(bool selected, uint8_t margin, uint16_t startY, uint16_t sizeX, uint16_t sizeY)
{
    // Draw rect
    if (selected)
    {
        M5.Lcd.fillRoundRect(margin, startY, sizeX, sizeY, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
        M5.Lcd.setTextColor(TEXT_COLOR);
    }
    else
    {
        M5.Lcd.fillRoundRect(margin, startY, sizeX, sizeY, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
        M5.Lcd.drawRoundRect(margin, startY, sizeX, sizeY, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
        M5.Lcd.setTextColor(TEXT_COLOR);
    }
}

void displayClearMainView(uint8_t offsetY)
{
    M5.Lcd.fillRect(0, 0, M5.Lcd.width(), M5.Lcd.height(), BACKGROUND_COLOR);
}

void displayClearTerminalView()
{
    M5.Lcd.fillRect(0, 0, M5.Lcd.width(), M5.Lcd.height() - 30, BACKGROUND_COLOR);
}

void displayMessageBox(std::string message)
{
    // Clear screen
    M5.Lcd.fillScreen(BACKGROUND_COLOR);

    // Draw box
    uint16_t boxWidth = M5.Lcd.width() - 40;
    uint16_t boxHeight = 30;
    uint16_t boxX = 20;
    uint16_t boxY = (M5.Lcd.height() - boxHeight) / 2;

    M5.Lcd.fillRoundRect(boxX, boxY, boxWidth, boxHeight, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
    M5.Lcd.drawRoundRect(boxX, boxY, boxWidth, boxHeight, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    // Print message
    M5.Lcd.setTextColor(TEXT_COLOR);
    M5.Lcd.setTextSize(1.5);
    M5.Lcd.setCursor(boxX + 10, boxY + (boxHeight / 2) - 8);
    M5.Lcd.printf(message.c_str());
}

unsigned int selectFromList(std::vector<std::string> items, unsigned int startIndex)
{
    uint8_t selectedIndex = startIndex;
    bool selectionMade = false;
    bool firstRender = true;

    while (!selectionMade)
    {

        // Wait for button input
        char input = promptInputHandler();

        switch (input)
        {
        case KEY_ARROW_UP:
        case KEY_ARROW_LEFT:
            if (selectedIndex > 0)
            {
                selectedIndex--;
            }
            firstRender = false;
            break;

        case KEY_ARROW_DOWN:
        case KEY_ARROW_RIGHT:
            if (selectedIndex < items.size() - 1)
            {
                selectedIndex++;
            }
            firstRender = false;
            break;

        case KEY_OK:
            return selectedIndex;
        case KEY_NONE:
            // No input, continue
            if (firstRender)
            {
                firstRender = false;
            }
            else
            {
                continue;
            }
        default:
            break;
        }

        // Clear screen
        displayClearMainView();

        // Display items
        for (size_t i = 0; i < items.size(); ++i)
        {
            bool isSelected = (i == selectedIndex);
            drawRect(isSelected, DEFAULT_MARGIN, DEFAULT_MARGIN + (i * 30), M5.Lcd.width() - 15, 25);
            M5.Lcd.setCursor(DEFAULT_MARGIN + 10, DEFAULT_MARGIN + 8 + (i * 30));
            M5.Lcd.print(items[i].c_str());
        }

        delay(100);
    }

    return selectedIndex;
}

std::string getInput(std::string prompt)
{
    bool firstRender = true;
    std::string ret = "";

    while (1)
    {

        // Wait for button input
        char input = promptInputHandler();

        switch (input)
        {
        case KEY_DEL:
            if (ret.size())
            {
                ret.pop_back();
            }
            break;
        case KEY_RETURN:
        case KEY_OK:
            if (firstRender)
            {
                continue;
            }
            return ret;
        case KEY_NONE:
            // No input, continue
            if (firstRender)
            {
                firstRender = false;
                break;
            }
            else
            {
                continue;
            }
        default:
            ret += input;
            break;
        }

        // Clear screen
        displayClearMainView();

        std::string sendString = ret;

        if (sendString.length() > 26)
        {
            sendString = sendString.substr(sendString.length() - 26);
        }

        // TODO print prompt to tell user what they are entering

        // Draw box
        uint16_t boxWidth = M5.Lcd.width() - 40;
        uint16_t boxHeight = 30;
        uint16_t boxX = 20;
        uint16_t boxY = (M5.Lcd.height() - boxHeight) / 2;

        M5.Lcd.fillRoundRect(boxX, boxY, boxWidth, boxHeight, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
        M5.Lcd.drawRoundRect(boxX, boxY, boxWidth, boxHeight, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

        // Print typed message
        M5.Lcd.setTextColor(TEXT_COLOR);
        M5.Lcd.setTextSize(1.5);
        M5.Lcd.setCursor(boxX + 10, boxY + (boxHeight / 2) - 8);
        M5.Lcd.print("> ");
        M5.Lcd.print(sendString.c_str());

        delay(100);
    }
}