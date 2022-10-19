// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

LinkLayer ll;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer ll = {serialPort,role,baudRate,nTries,timeout};
}
