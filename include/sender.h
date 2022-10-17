#ifndef _SENDER_H
#define _SENDER_H

#include <unistd.h>
#include <signal.h>
#include "./utils.h"

int sendControl(int fd, unsigned char controlField, unsigned char response);


#endif