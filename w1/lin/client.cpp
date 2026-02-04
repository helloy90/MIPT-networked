#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <netdb.h>

#include "socket_tools.h"

std::atomic<bool> running{true};


std::string buffered_msg;
std::vector<std::string> messages = {
    "https://youtu.be/dQw4w9WgXcQ?si=Ox1lQULEyKpisqd",
    "Never gonna give you up",
    "Never gonna let you down",
    "Never gonna run around and desert you",
    "Never gonna make you cry",
    "Never gonna say goodbye",
    "Never gonna tell a lie and hurt you",
    "..."};

const char *port = "2026";
addrinfo resAddrInfo;
int sfd;

void receive_messages()
{
  int message_num = -1;
  while (running)
  {
    message_num = (message_num + 1) % messages.size();
    std::cout << "\rRick: " << messages[message_num] << std::endl;
    std::cout << "> " << buffered_msg << std::flush;

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}

void handle_input()
{
  char ch = std::cin.get();
  bool is_printable = ch >= 32 && ch <= 126;
  bool is_backspace = ch == 127 || ch == '\b';
  bool is_enter = ch == '\n';

  if (is_printable)
  {
    buffered_msg += ch;
    std::cout << ch << std::flush;
  }
  else if (is_backspace)
  {
    if (!buffered_msg.empty())
    {
      buffered_msg.pop_back();
      std::cout << "\b \b" << std::flush;
    }
  }
  else if (is_enter)
  {
    if (buffered_msg == "/quit")
    {
      running = false;
      std::cout << "\nExiting...\n";
    } else if (!buffered_msg.empty())
    { 
      std::cout << "\rYou sent: " << buffered_msg << "\n";
      ssize_t res = sendto(sfd, buffered_msg.c_str(), buffered_msg.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
      if (res == -1)
      {
        std::cout << strerror(errno) << std::endl;
      }

      buffered_msg.clear();
      std::cout << "> " << std::flush;
    }
  }
}

int main(int argc, const char **argv)
{
  std::cout << "ChatClient - Type '/quit' to exit\n"
            << "> ";

  sfd = create_client("localhost", port, &resAddrInfo);
  if (sfd == -1)
  {
    std::cout << "Failed to create a socket\n";
    return 1;
  }

  std::thread receiver(receive_messages);
  while (running)
  {
    handle_input();
  }

  receiver.join();
  return 0;
}
