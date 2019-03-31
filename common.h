#ifndef COMMON_H
#define COMMON_H
//test2
//test3
int initDB();
int findById(int _id, int *_index, unsigned char *ucOut, int *iLen);
//int insertToDB(unsigned char *pucRecvData);
int insertToDB(unsigned char *pucRecvData);
int anotherPacketIsExist(unsigned char *_pucRecvData);
int getAnotherPacketData(unsigned char *_pucRecvData, unsigned char *pucOut, int *totalLen);

#endif
