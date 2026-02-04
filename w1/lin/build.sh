#!/bin/bash

mkdir -p bin

g++ server.cpp socket_tools.cpp -std=c++17 -o bin/server
g++ client.cpp socket_tools.cpp -std=c++17 -o bin/client