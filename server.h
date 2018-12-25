#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <sys/socket.h>
#include "common.h"
#include <string.h>


//extern int t_force_socket_exit;	                             /* force socket exit */

int setupServer(int iPort);
int	convetHexToStr(unsigned char *toBroadcast, int _iLen, char *pcOut);
int packetData(unsigned char *pucRecvData, unsigned char *pucBlobData, int _len, char *pcOut, int *_outLen);
int parseData(unsigned char *pucIn, int len, int *pusId, int *ucIndex);


#endif