#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<stdio.h>
#include<unistd.h>

#define SEC *1000000

unsigned char *dmx_values;
int nbr_values = 513;


int main(int argc, char *argv[])
{
	int i;
	int brightness=0;
	int shmid;

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

	while (1)
        {
                for (brightness=0;brightness<255;brightness=brightness+1)     // Fade up
                {
                        for (i=1;i<=512;i++)
                                dmx_values[i]=brightness;                       // copy same v$
                	usleep(100000);
		        //numbits=build_dmx_packet();                             // put the bit$
                        //send_dmx_packet(numbits);
                }

                for (brightness=255;brightness>0;brightness=brightness-1)       // Fade down
                {
                        for (i=1;i<=512;i++)
                                dmx_values[i]=brightness;                       // copy same v$
               		usleep(50000);
			//numbits=build_dmx_packet();                             // put the bit$
                        //send_dmx_packet(numbits);
                }
        }

	return 0;
}
