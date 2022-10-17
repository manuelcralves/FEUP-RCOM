#ifndef _RECEIVER_H
#define _RECEIVER_H

#include <unistd.h>
#include <signal.h>
#include "./utils.h"

int receiveControl(int fd, unsigned char controlField, unsigned char response);

#endif