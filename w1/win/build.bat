@echo off

if not exist bin\ (
    mkdir bin
)

clang++ server_main.cpp socket_tools.cpp Server.cpp -std=c++20 -o bin/server.exe -lws2_32
clang++ client.cpp socket_tools.cpp -std=c++20 -o bin/client.exe -lws2_32
