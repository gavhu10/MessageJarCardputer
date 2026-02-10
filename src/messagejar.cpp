#include "messagejar.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <memory>
#include <string>
#include <vector>
#include <map>

using std::make_shared;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

#define SERVER_URL "https://messagejar.pythonanywhere.com/api"

bool check_resp(const std::string &resp)
{
    // 1. Find the first non-whitespace character (the opening '{')
    auto first = std::find_if(resp.begin(), resp.end(), [](unsigned char ch)
                              { return !std::isspace(ch); });

    // If string is empty or only whitespace
    if (first == resp.end() || *first != '{')
    {
        return true;
    }

    // 2. Look for the "e" key immediately following the '{'
    // We skip whitespace again in case there is a space after the '{'
    auto after_brace = std::next(first);
    auto key_start = std::find_if(after_brace, resp.end(), [](unsigned char ch)
                                  { return !std::isspace(ch); });

    // Check if the next sequence is "e":
    const std::string error_key = "\"e\":";

    // Check if the remaining string is long enough and matches
    if (std::distance(key_start, resp.end()) >= error_key.size())
    {
        if (std::equal(error_key.begin(), error_key.end(), key_start))
        {
            return false;
        }
    }

    return true;
}

unique_ptr<string> request(const string &endpoint, const std::map<string, string> &kv)
{
    HTTPClient http;
    string url = string(SERVER_URL) + endpoint;

    JsonDocument doc;

    for (const auto &item : kv)
    {
        doc[item.first] = item.second;
    }

    String jsonBody;
    serializeJson(doc, jsonBody);

    http.begin(url.c_str());

    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(jsonBody);

    if (httpCode > 0)
    {
        String payload = http.getString();
        auto result = unique_ptr<string>(new string(payload.c_str()));
        http.end();
        return result;
    }

    http.end();
    return nullptr;
}

Message::Message(const std::map<string, string> &data)
{
    try
    {
        author = data.at("author");
        content = data.at("content");
        timestamp = data.at("created");
        message_id = data.at("id");
    }
    catch (const std::out_of_range &)
    {
        author = "";
        content = "error parsing message";
        timestamp = "";
        message_id = "";
    }
}

string Message::as_string() const
{
    // return "[" + timestamp + "] " + author + ": " + content;
    return author + ": " + content + "\n";
}

MessageJar::MessageJar(string token) : token(token) {}

bool MessageJar::check()
{
    auto response = request("/token/username", {{"token", token}});
    if (!response || !check_resp(*response))
    {
        return false;
    }
    return true;
}

bool MessageJar::create_user(string username, string password)
{
    auto response = request("/user/create", {{"username", username}, {"password", password}});
    if (!response || !check_resp(*response))
    {
        return false;
    }
    return true;
}

shared_ptr<vector<string>> MessageJar::get_rooms()
{

    auto response = request("/rooms/list", {{"token", token}});
    if (!response || !check_resp(*response))
    {
        return nullptr;
    }

    auto rooms = make_shared<vector<string>>();
    JsonDocument doc;
    deserializeJson(doc, response->c_str()); // TODO check for error
    JsonArray arr = doc.as<JsonArray>();
    for (JsonVariant v : arr)
    {
        rooms->push_back(v.as<string>());
    }

    return rooms;
}

shared_ptr<vector<Message>> MessageJar::get_messages(string room, int latest)
{

    auto response = request("/get", {{"token", token}, {"room", room}});
    if (!response || !check_resp(*response))
    {
        return nullptr;
    }

    auto messages = make_shared<vector<Message>>();
    JsonDocument doc;
    deserializeJson(doc, response->c_str());
    JsonArray arr = doc.as<JsonArray>(); // TODO check for error
    for (JsonVariant v : arr)
    {
        std::map<string, string> msg_data;
        JsonDocument msg_doc;
        deserializeJson(msg_doc, v.as<string>().c_str());
        for (auto pair : msg_doc.as<JsonObject>())
        {
            msg_data[pair.key().c_str()] = pair.value().as<string>();
        }
        messages->push_back(Message(msg_data));
    }

    return messages;
}

bool MessageJar::send(string room, string content)
{

    auto response = request("/send", {{"token", token}, {"room", room}, {"message", content}});
    return (response && check_resp(*response));
}

bool MessageJar::create_room(string room_name)
{
    auto response = request("/rooms/create", {{"token", token}, {"room", room_name}});
    request(*response, {{}});
    if (!response || !check_resp(*response))
    {
        return false;
    }
    return (response && check_resp(*response));
}