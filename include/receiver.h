#ifndef _RECEIVER_H
#define _RECEIVER_H

#include <unistd.h>
#include <signal.h>
#include "./utils.h"

unsigned char* destuffing(const unsigned char* buf, int *bufSize);

#endif