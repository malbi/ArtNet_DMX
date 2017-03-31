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
#include <errno.h>

#define SEC *1000000
#define PORT 6454
#define MAX_BUFF_ARTNET 530
#define ART_NET_ID "Art-Net\0"
#define ART_POLL 0x2000
#define ART_DMX 0x5000


unsigned char *dmx_values;
int nbr_values = 514;


typedef struct ArtNet ArtNet_t;
struct ArtNet{
	uint16_t opcode;
	uint8_t proVer;
	uint8_t sequence;
	uint8_t physical;
	uint16_t universe;
	uint16_t length;
	//uint8_t *dmx_values;
	};



int main(int argc, char *argv[])
{
	int i = 0, j = 0; 	//iterators
	int shmid = 0;		//shared memory id
	int sock = 0;		//socket file descriptor

	struct sockaddr_in serv_addr, cli_addr;
	char buff[MAX_BUFF_ARTNET];
	socklen_t len = sizeof(cli_addr);

	ArtNet_t artnet;

        //Server configuration:
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);


	//Obtain the shared memory
	if((shmid = shmget((key_t)1337, nbr_values*sizeof(uint8_t), IPC_CREAT | 0666)) < 0)
		fprintf(stderr, "shmget: %s\n", strerror(errno));

	//Attach dmx_values to the shared memory
	if((dmx_values = shmat(shmid, NULL, 0)) == (uint8_t*) -1)
		fprintf(stderr, "shmat: %s\n", strerror(errno));

	//UDP configuration:
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		fprintf(stderr, "socket: %s\n", strerror(errno));

	//Bind the sockect
	if( bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
		fprintf(stderr, "bind: %s\n", strerror(errno));


	while (1)
        {
		if(recvfrom( sock, buff, sizeof(buff), 0, (struct sockaddr*)&cli_addr, &len) > 0)
		{
			for(i = 0 ; i < 8 ; i++)
			{
				if( buff[i] != ART_NET_ID[i]) 
					return 0;
			}

			artnet.opcode = buff[8] | buff[9] << 8;
			if(artnet.opcode == ART_DMX){
				artnet.sequence = buff[12];
				artnet.universe = buff[14] | buff[15] << 8;
				artnet.length = buff[17] | buff[16] << 8;

				for (i = 0; i < sizeof(dmx_values); i ++) 
				{
					j = i + 17;
					dmx_values[i] = buff[j];
					//a tester: associer seulement le pointeur:
					//dmx_values = buff + 17; //17: debut du packet dmx
				}
			}
			if(artnet.opcode == ART_POLL)
				;//implementer la réponse art poll
		}
	}
	return 0;
}
