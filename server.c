#include <sys/syscall.h>
#include "server.h"
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include "errno.h"

#include <sys/stat.h>


/*
 * function  ： pthread to handle ucRecvDataBuf
 * parameter ： ucRecvDataBuf
 * return    ：
 */

static void *pthread_handle(void *arg)
{
	unsigned char *pucTmp = (unsigned char *)(arg);
	for(int i = 0; i < 500;i++){
		printf("%02x ", pucTmp[i]);
	}
	printf("\n");
#if 0
	printf("The thread's tid is %d\n", syscall(SYS_gettid));
	//解析数据提取交易id
	int id = 0;
	int index = 0;
	unsigned char pucOut[1024] = {};
	int iLen = 0;
	parseData(pucTmp, 1024, &id, &index);
	//查询数据库
	printf("id =%d\n", id);
	int ret = findById(id, &index, pucOut, &iLen);
	if (ret == 0/*not found*/) {
		//没有找到则把该条交易信息写入数据库。
		insertToDB(pucTmp);
	}
	else if (ret == 1/*找到*/) {
		//合并
		char toBroadcast[1024] = {};
		int outLen = 0;
		packetData(pucTmp, pucOut, iLen, toBroadcast, &outLen);
		char pcStr[1024] = {0};
		convetHexToStr(toBroadcast, outLen, pcStr);
		//广播数据
		char cmd[1024] = {};
		sprintf(cmd, "./qtum-cli -testnet sendrawtransaction %s", pcStr);
		printf("cmd =%s.\n", cmd);
		system(cmd);
	}
	else {
		//出错
		printf("pthread_handle error\n");
	}

	pthread_exit((void *)0);
#endif
	//write received data to db
	if (0 != insertToDB(pucTmp)) {
		printf("write error\n");
		pthread_exit((void *)(-1));
	}
	//judge another packet is exist
	if (0 != anotherPacketIsExist(pucTmp)) {
		printf("packet is not exist\n");
		pthread_exit((void *)(1));
	}
	//合并
	unsigned char pucOut[1024] = {};
	int iLen = 0;
	char toBroadcast[1024] = {};
	int outLen = 0;
	getAnotherPacketData(pucTmp, pucOut, &iLen);
	packetData(pucTmp, pucOut, iLen, toBroadcast, &outLen);
	char pcStr[1024] = { 0 };
	convetHexToStr(toBroadcast, outLen, pcStr);
	//广播数据
	char cmd[1024] = {};
	sprintf(cmd, "./qtum-cli -testnet sendrawtransaction %s", pcStr);
	printf("cmd =%s.\n", cmd);
	system(cmd);
	pthread_exit((void *)(0));
}

/*
 * function  ： setup socket server
 * parameter ： port
 * return    ： 0：success ，-1：failure
 */

int setupServer(int iPort)
{
	struct sockaddr_in sin;
	struct sockaddr_in pin;
	fd_set readfds;
	struct timeval timeout;
	int sock_descriptor = -1;
	int temp_sock_descriptor = -1;
	socklen_t sockaddrlen;

	sock_descriptor = socket(PF_INET, SOCK_STREAM, 0);
	if (sock_descriptor == -1)
		return -1;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(iPort);
	//Set the socket as non-blocking
	int flags = fcntl(sock_descriptor, F_GETFL, 0);
	fcntl(sock_descriptor, F_SETFL, flags | O_NONBLOCK);
	int t_force_socket_exit = 0;

		while (1)
		{
			if (t_force_socket_exit == 1)
				goto END;
			if (bind(sock_descriptor, (struct sockaddr*)&sin, sizeof(sin)) == -1) //与选定的端口绑定
			{
				sleep(1);
				continue;
			}
			break;
		}
		if (listen(sock_descriptor, 100) == -1)//最多可同时100个连接
			goto END;
		//等待连接
		while (1)
		{
			//查询是否退出
			if (t_force_socket_exit == 1)
				break;

			//检查套接字是否处于读状态，读状态表示有客户端正在连接(connect)
			FD_ZERO(&readfds);
			FD_SET(sock_descriptor, &readfds);       // add sock to set
			timeout.tv_sec = 2;
			timeout.tv_usec = 0;
			if (select(sock_descriptor + 1, &readfds, NULL, NULL, &timeout) <= 0)
				continue;
			if (!FD_ISSET(sock_descriptor, &readfds))
				continue;

			sockaddrlen = sizeof(pin);
			temp_sock_descriptor = accept(sock_descriptor, (struct sockaddr*)&pin, &sockaddrlen);
			if (temp_sock_descriptor == -1 && errno == EAGAIN)
				continue;
			if (temp_sock_descriptor != -1)
			{
				unsigned char ucRecvDataBuf[1024]={0};
				//read socket
				read(temp_sock_descriptor, ucRecvDataBuf, sizeof(ucRecvDataBuf));
				//create thread to handle data;
				//int ifd = open("./0.dat", O_RDONLY);
				//read(ifd, ucRecvDataBuf,1024);

				pthread_t tid;
				if (0 != pthread_create(&tid, NULL, pthread_handle, ucRecvDataBuf)) {
					printf("create thread error.\n");
				}
				pthread_detach(tid);
			}
			else
			{
				close(temp_sock_descriptor);
			}
		}

	END:
		if (sock_descriptor != -1)
			close(sock_descriptor);
	return 0;  //success
}

/*
 * function     ： pthread to handle ucRecvDataBuf
 * parameter[0] ： point to data
 * parameter[1] ： len
 * parameter[2] ： transaction id
 * return       ： 0:success , -1:failure
 */

int parseData(unsigned char *pucIn, int len, int *pusId, int *ucIndex)
{
	unsigned char *tmp = pucIn;
	if (tmp[25] !=0 /* transaction error */) {
		printf("data error.\n");
		return -1;
	}
	/*取得id的高字节和低字节的数据*/
	unsigned char ucIdLow = tmp[39];
	unsigned char ucIdHigh = tmp[42];
	*pusId |= ucIdLow;
	*pusId = (*pusId << 8) | ucIdHigh;
	/*多签数据的index*/
	*ucIndex = tmp[41];
	return 0;
}

/*
 * function     ： packet two together
 * parameter[0] ： point to data
 * parameter[1] ： blob data
 * parameter[2] ： blob data len;
 * parameter[3] :  output data
 * parameter[4] :  output data len;
 * return       ： 0:success , -1:failure
 */

int packetData(unsigned char *pucRecvData, unsigned char *pucBlobData, int _len, char *pcOut, int *_outLen)
{
	//解析recvdata的长度
	//解析recvdata的index
	//更具index指定组合数据的位置，决定是recvdata放在前边还是，blob数据放在前边
	//转换成字符串用于广播

	int iRecvLen = 0;
	iRecvLen |= pucRecvData[35];
	iRecvLen = ((iRecvLen << 8) | pucRecvData[36])-6;
	int iIndex = pucRecvData[41];
	if (iIndex==1) {
		memcpy(pcOut, pucRecvData+43, iRecvLen);
		memcpy(pcOut+ iRecvLen, pucBlobData, _len);
	}else if (iIndex==2) {
		memcpy(pcOut, pucBlobData, _len);
		memcpy(pcOut+ _len, pucRecvData + 43, iRecvLen);
	}else {
		//出错
		printf("packetData error\n");
		return -1;
	}
	*_outLen = iRecvLen + _len;
	return 0;
}
int	convetHexToStr(unsigned char *toBroadcast, int _iLen, char *pcOut)
{
	for (int i = 0; i < _iLen; i++) {
		unsigned char tmp = toBroadcast[i];
		char cLow = tmp & 0x0f;
		if ((cLow>=0)&&(cLow<=9)) {
			cLow = cLow + '0';
		}else if((cLow>=10)&&(cLow<=15)) {
			cLow = cLow - 10 + 'a';
		}
		pcOut[i * 2+1] = cLow;
		char cHig = tmp >> 4;
		if ((cHig >= 0) && (cHig <= 9)) {
			cHig = cHig + '0';
		}
		if ((cHig >= 10) && (cHig <= 15)) {
			cHig = cHig - 10 + 'a';
		}
		pcOut[i * 2] = cHig;
	}
}



