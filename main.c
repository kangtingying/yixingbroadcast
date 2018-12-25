
#include <stdio.h>
#include "common.h"
#include "server.h"
int main(int argc, char **argv)
{
	//init db
	initDB();
	//setup socket server
	setupServer(60000);

	return 0;
}
