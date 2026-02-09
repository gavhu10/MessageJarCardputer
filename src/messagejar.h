#ifndef MESSAGEJAR_H
#define MESSAGEJAR_H

#include <string>
#include <memory>
#include <vector>
#include <map>

using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

unique_ptr<string> request(const string &endpoint, const std::map<string, string> &kv);

class Message
{
public:
    Message(const std::map<string, string> &data);
    string as_string() const;

private:
    string author;
    string content;
    string timestamp;
    string message_id;
};

class MessageJar
{
public:
    MessageJar(string token);
    bool check();
    bool create_user(string username, string password);
    shared_ptr<vector<string>> get_rooms();
    shared_ptr<vector<Message>> get_messages(string room, int latest = 0);
    bool send(string room, string content);
    bool create_room(string room_name);

private:
    string token;
};

#endif // MESSAGEJAR_H
