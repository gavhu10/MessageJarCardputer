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

#define SERVER_URL "https://gavhu10.pythonanywhere.com/api/"

unique_ptr<string> request(const string &endpoint, const std::map<string, string> &kv)
{
    HTTPClient http;
    string url = string(SERVER_URL) + endpoint + "?";

    for (auto &pair : kv)
    {
        url += pair.first + "=" + pair.second + "&";
    }

    http.begin(url.c_str());
    int httpCode = http.GET();

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
        timestamp = data.at("timestamp");
        message_id = data.at("message_id");
    }
    catch (const std::out_of_range &)
    {
        author = "";
        content = "error loading message";
        timestamp = "";
        message_id = "";
    }
}

string Message::as_string() const
{
    return "[" + timestamp + "] " + author + ": " + content;
}

MessageJar::MessageJar(string username, string password) : username(username), password(password) {}

bool MessageJar::check()
{
    auto response = request("/api-manage", {{"username", username}, {"password", password}, {"action", "verify_user"}});
    return response != nullptr;
}

bool MessageJar::create_user()
{
    auto response = request("/api-manage", {{"username", username}, {"password", password}, {"action", "new_user"}});
    return response != nullptr;
}

shared_ptr<vector<string>> MessageJar::get_rooms()
{
    if (!check())
    {
        return nullptr;
    }

    auto response = request("/api-manage", {{"username", username}, {"password", password}, {"action", "list_rooms"}});
    if (!response)
    {
        return nullptr;
    }

    auto rooms = make_shared<vector<string>>();
    JsonDocument doc;
    deserializeJson(doc, response->c_str());
    JsonArray arr = doc.as<JsonArray>();
    for (JsonVariant v : arr)
    {
        rooms->push_back(v.as<string>());
    }

    return rooms;
}

shared_ptr<vector<Message>> MessageJar::get_messages(string room, int latest)
{
    if (!check())
    {
        return nullptr;
    }

    auto response = request("/get_messages", {{"username", username}, {"password", password}, {"room", room}});
    if (!response)
    {
        return nullptr;
    }

    auto messages = make_shared<vector<Message>>();
    JsonDocument doc;
    deserializeJson(doc, response->c_str());
    JsonArray arr = doc.as<JsonArray>();
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
    if (!check())
    {
        return false;
    }

    auto response = request("/api-send", {{"username", username}, {"password", password}, {"room", room}, {"message", content}});
    return response != nullptr;
}

bool MessageJar::create_room(string room_name)
{
    if (!check())
    {
        return false;
    }

    auto response = request("/api-manage", {{"username", username}, {"password", password}, {"action", "create_room"}, {"room", room_name}});
    return response != nullptr;
}