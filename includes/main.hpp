#pragma once

#define OK 0
#define ERROR 1

#define PORT 3000


// Thorben
// in IO.cpp
#define CGI_TIMEOUT 5
// in Epoll.cpp
#define EPOLL_TIMEOUT_MS 100

// Benny
#include <csignal>

extern volatile sig_atomic_t stopSignal;

#define DEBUG_MODE false
#define GET "GET"
#define HEAD "HEAD"
#define POST "POST"
#define TELNETSTOP "stopFromTelnet"


// Sumon
