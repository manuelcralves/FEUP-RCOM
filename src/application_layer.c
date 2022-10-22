// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "utils.h"
#include <math.h>
#include <sys/stat.h>

int fd;
LinkLayerRole myRole;
LinkLayer ll;

LinkLayerRole getRole(const char *role)
{
  if (strcmp(role, "tx") == 0)
    return LlTx;
  else if (strcmp(role, "rx") == 0)
    return LlRx;
  else
  {
    printf("Invalid Role!\n");

    return (LinkLayerRole)NULL;
  }
}

int appRead (int fd) {
    unsigned char buffer[255];
    int res, currentIndex = 0;
    off_t fileLengthStart = 0, currentPosition = 0, dataLength = 0, fileLengthEnd = 0;
    unsigned int nameSize = 0;

    /*Receive start packet*/
    if ((res = llread(buffer))<0) {
        perror("llread");
        exit(-1);
    }

    if (buffer[currentIndex++] != START_PKT){
        perror("start packet");
        exit(-1);
    }

    /*File length*/
    if (buffer[currentIndex++] != FILE_SIZE){
        perror("start-wrong type");
        exit(-1);
    }

    int length = buffer[currentIndex++];

    for (size_t i = 0; i < length;i++){
        fileLengthStart = (fileLengthStart*256) + buffer[currentIndex++]; /* hexa to dec*/
    }

    /*File name*/

    if (buffer[currentIndex++] != FILE_NAME){
        perror("start-wrong type");
        exit(-1);
    }

    nameSize = buffer[currentIndex++];
    char *name = (char *)malloc(nameSize);
    for (size_t i = 0; i < nameSize;i++){
        name[i] = buffer[currentIndex++];
    }


    int file_fd;

    if ((file_fd = open(name,O_WRONLY | O_CREAT,0444)) < 0 ){ /* no write permission for anybody except the current process*/
        free(name);
        perror("open file");
        exit(-1);
    }

    while (currentPosition != fileLengthStart){
        if ((res = llread(buffer)) < 0) {
            free(name);
            close(file_fd);
            perror("llread");
            exit(-1);
        }

        if (!res) continue;

        currentIndex = 0;
        if (buffer[currentIndex++] != DATA_PKT) {
            free(name);
            close(file_fd);
            perror("FIle not fully received");
            exit(-1);
        }

        currentIndex++;
        dataLength = buffer[currentIndex] * 256 + buffer[currentIndex+1];

        currentIndex += 2;
        char * data = (char *)malloc(dataLength);
        for(size_t i = 0; i < dataLength;i++){
            data[i] = buffer[currentIndex++];
        }

        currentPosition += dataLength;

        if (write(file_fd,data,dataLength) < 0){
            free(data);
            free(name);
            close(file_fd);
            perror("Write to file");
            exit(-1);
        }

        free(data);

    }

    close (file_fd);

    /*Receive end packet*/
    currentIndex = 0;
    if ((res = llread(buffer)) < 0) {
        free(name);
        perror("llread");
        exit(-1);
    }

    if (buffer[currentIndex++] != END_PKT){
        free(name);
        perror("WRong type");
        exit(-1);
    }

    /*File length*/
    length = buffer[currentIndex++];
    for (size_t i = 0; i < length;i++){
        fileLengthEnd = (fileLengthEnd * 256) + buffer[currentIndex++]; // hexa to dec
    }

    if (fileLengthEnd != fileLengthStart){
        free(name);
        perror("Wrong length");
        exit(-1);
    }   

    /*FIle name*/
    if(buffer[currentIndex++] != FILE_NAME){
        free(name);
        perror("End - Wrong type");
        exit(-1);
    }

    if (buffer[currentIndex++] != nameSize){
        free(name);
        perror("WRong name size");
        exit(-1);
    }

    for (size_t i = 0; i < nameSize;i++){
        if (name[i] != buffer[currentIndex++]){
            free(name);
            perror("Wrong name");
            exit(-1);
        }
    }

    free(name);
    return 0;

}

int appWrite(int fd, const char * name) {
    struct stat fileInfo;
    unsigned int currentIndex = 0;
    int resW = 0, resR = 0, seqNum = 0;
    unsigned char countBytes;

    if (stat(name,&fileInfo) < 0) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    off_t length = fileInfo.st_size;
    off_t auxLength = length;

    unsigned char controlPacket[255];
    controlPacket[currentIndex++] = START_PKT; /* Control */
    controlPacket[currentIndex++] = FILE_SIZE; /* Type*/

    while (auxLength > 0) {
        countBytes++;
        auxLength /= 255;
    }
    controlPacket[currentIndex] = countBytes; /*Length , number of bytes needed to represent the file length*/

    for (size_t i = controlPacket[currentIndex++]; i > 0;i--){
        controlPacket[currentIndex++] = length & 0xFF;
        length = length >> 8;
    }

    controlPacket[currentIndex++] = FILE_NAME;
    controlPacket[currentIndex++] = strlen(name);
    for (size_t i = 0; i < strlen(name);i++){
        controlPacket[currentIndex++] = name[i];
    }

    /* Send start packet*/
    llwrite(controlPacket,currentIndex);

    /*Open transmission file*/
    int file_fd;
    if ((file_fd = open(name,O_RDONLY)) < 0) {
        perror("open file");
        exit(-1);
    }

    unsigned char data[MAX_PAYLOAD_SIZE];
    unsigned char* dataPacket = (unsigned char*)malloc(MAX_PAYLOAD_SIZE);
    off_t currentPosition = 0;

    /* Send data packets */
    while (currentPosition != length) {
        if ((resR = read(file_fd,data,MAX_PAYLOAD_SIZE)) < 0) {
            free(dataPacket);
            exit(-1);
        }

        dataPacket = (unsigned char*)realloc(dataPacket,resR+4);
        dataPacket[0] = DATA_PKT;
        dataPacket[1] = seqNum % 255;
        dataPacket[2] = (resR/256);
        dataPacket[3] = (resR % 256);
        for (size_t i = 0; i <resR;i++){
            dataPacket[4+i] = data[i];
        }
        if((resW = llwrite(dataPacket,resR+4)) < 0){
            free(dataPacket);
            exit(-1);
        }

        currentPosition += resR;
        seqNum++;
    }

    controlPacket[0] = END_PKT;
    llwrite(controlPacket,currentIndex);
    free(dataPacket);

    return 0;

}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{   

    int res = 0;

    LinkLayer ll;
    strcpy(ll.serialPort,serialPort);
    ll.role = getRole(role);
    myRole = getRole(role);
    ll.baudRate = baudRate;
    ll.nRetransmissions = nTries;
    ll.timeout = timeout;

    fd = llopen(ll);
    printf("fd: %d\n",fd);

    if (myRole == LlRx) {
        printf("role rec\n");
        res = appRead(fd);
        llclose(0);
    }

    else if (myRole == LlTx){
        printf("role trans\n");
        res = appWrite(fd,filename);
        llclose(0);
    }


}
