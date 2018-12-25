#include <sqlite3.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>


pthread_mutex_t g_dbMtx = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_lock(&mtx);
//pthread_mutex_unlock(&mtx);


/*
 * function     ： db init
 * parameter[0] ： 
 * parameter[1] ： 
 * parameter[2] ： 
 * return       ： 0:success , -1:failure
 */

int initDB()
{
	sqlite3 *db;
	sqlite3_stmt *res;
	char *err_msg = 0;
	int ret = sqlite3_open("./broadcast.db", &db);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
	printf("create db succcess.\n");
	/*create table of transaction data*/
	char createSql[] = "create table if not exists broadcast_data(ID integer,type INTEGER,indx INTEGER,num INTEGER,data BLOB);";
	char *errmsg = NULL;
	ret = sqlite3_exec(db, createSql, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		return -1;
	}
	printf("create table broadcast_data success.\n");
	sqlite3_free(errmsg);
	sqlite3_close(db);
	return 0;
}

/*
 * function     ： find data by id
 * parameter[0] ： id
 * parameter[1] ： output transaction index
 * parameter[2] ： point to data
 * parameter[3] ： output len
 * return       ： 1:found, 0:not found , -1:failure
 */

int findById(int _id, int *_index, unsigned char *ucOut, int *iLen)
{
	sqlite3 *db;
	sqlite3_stmt *pStmt;
	pthread_mutex_lock(&g_dbMtx);
	int ret = sqlite3_open("./broadcast.db", &db);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "findById Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(&g_dbMtx);
		return -1;
	}
	char sql[1024] = {};
	sprintf(sql, "SELECT count(*) FROM broadcast_data WHERE ID = %d;", _id);
	ret = sqlite3_prepare_v2(db, sql, -1, &pStmt, 0);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement\n");
		sqlite3_finalize(pStmt);
		sqlite3_close(db);
		pthread_mutex_unlock(&g_dbMtx);
		return -1;
	}
	ret = sqlite3_step(pStmt);
	int iNum = 0;
	iNum = sqlite3_column_int(pStmt, 0);
	if (iNum==0) {
		printf("not found by id\n");
		sqlite3_finalize(pStmt);
		sqlite3_close(db);
		pthread_mutex_unlock(&g_dbMtx);
		return 0;
	}
	sqlite3_reset(pStmt);
	sprintf(sql, "SELECT * FROM broadcast_data WHERE ID = %d;", _id);
	ret = sqlite3_prepare_v2(db, sql, -1, &pStmt, 0);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement\n");
		sqlite3_finalize(pStmt);
		sqlite3_close(db);
		pthread_mutex_unlock(&g_dbMtx);
		return -1;
	}
	ret = sqlite3_step(pStmt);
	*_index = sqlite3_column_int(pStmt, 2);
	printf("read blob index =%d\n", *_index);
	const void * pReadBolbData = sqlite3_column_blob(pStmt, 4);
	*iLen = sqlite3_column_bytes(pStmt, 4);
	printf("read blob len =%d\n", *iLen);
	memcpy(ucOut, pReadBolbData, *iLen);

	sqlite3_finalize(pStmt);
	sqlite3_close(db);
	pthread_mutex_unlock(&g_dbMtx);
	return 1;
}

/*
 * function     ： insert into db
 * parameter[0] ： received data from socket
 * parameter[1] ： data len
 * parameter[2] ： 
 * return       ： 0:success , -1:failure
 */

int insertToDB(unsigned char *pucRecvData)
{
	sqlite3 *db;
	sqlite3_stmt *pStmt;
	pthread_mutex_lock(&g_dbMtx);
	int ret = sqlite3_open("./broadcast.db", &db);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "insertToDB Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
	char sql[1024] = {};
	unsigned char *tmp = pucRecvData;
	/*取得id的高字节和低字节的数据*/
	unsigned char ucIdLow = tmp[39];
	unsigned char ucIdHigh = tmp[42];
	int id = 0;
	id |= ucIdLow;
	id = (id << 8) | ucIdHigh;
	int indx = tmp[41];
	int type = tmp[38];
	int num = tmp[40];
	int dwLen = 0;
	dwLen |= tmp[35];
	dwLen = ((dwLen << 8)| tmp[36]) -6;
	sprintf(sql, "INSERT INTO broadcast_data values(%d,%d,%d,%d,?);", id, type, indx, num);
	ret = sqlite3_prepare_v2(db, sql, -1, &pStmt, 0);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "insertToDB Failed to prepare statement\n");
		sqlite3_finalize(pStmt);
		sqlite3_close(db);
		pthread_mutex_unlock(&g_dbMtx);
		return -1;
	}
	ret = sqlite3_bind_blob(pStmt, 1, &tmp[43], dwLen, NULL);
	printf("insert blob %d\n", ret);
	ret = sqlite3_step(pStmt);
	if (ret != SQLITE_DONE) {
		printf("step error %d\n", ret);
		sqlite3_finalize(pStmt);
		sqlite3_close(db);
		pthread_mutex_unlock(&g_dbMtx);
		return -1;
	}
	sqlite3_finalize(pStmt);
	sqlite3_close(db);
	pthread_mutex_unlock(&g_dbMtx);
	printf("insert success\n");
	return 0;
}
