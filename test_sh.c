#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>


#define SEC *1000000
#define PORT 6454
#define MAX_BUFF_ARTNET 530
#define ART_NET_ID "Art-Net\0"

unsigned char *dmx_values;
int nbr_values = 513;


int main(int argc, char *argv[])
{
	int i = 0;
	int brightness=0;
	int shmid = 0;
	int sock = 0;
	int j = 0;
	int received_bytes = 0;
	struct sockaddr_in serv_addr, cli_addr;
	char buff[MAX_BUFF_ARTNET];
	socklen_t len = sizeof(cli_addr);

	if((shmid = shmget((key_t)1337, nbr_values*sizeof(unsigned char), IPC_CREAT | 0666)) < 0)
	{
		perror("shmget");
		return 1;
	}

	if((dmx_values = shmat(shmid, NULL, 0)) == (unsigned char*) -1)
	{
		perror("shmat");
		return 1;
	}

	//gestion communication
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}

	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	if( bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
	{
		perror("bind");
		return 1;
	}

	while (1)
        {
		if(recvfrom( sock, buff, sizeof(buff), 0, (struct sockaddr*)&cli_addr, &len) > 0)
		{
			for(i = 0 ; i < 8 ; i++)
			{
				if( buff[i] != ART_NET_ID[i]) 
					return 0;
			}
			for (i = 0; i < sizeof buff - 1; i ++) 
			{
				j = i + 17;
				dmx_values[i] = buff[j];
			}
		}
	}
	return 0;
}
