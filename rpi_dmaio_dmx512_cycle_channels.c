/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
Yes, you can remove this License and stick GPL BSD or APACHE License here instead
*/


/*
 rpi_dmaio_dmx512_cycle_channels.c 
 Author: Jonathan Andrews 
 (jon HAT jonshouse DOOOT co DOOOT uk)

 Test code to generate DMX512 packets.
 Cycles specified DMX channels.

 Version 0.2
 Created 16 May 2014
 Last modified 23 May 2014

 Raspberry Pi specific code using DMA I/O
 Requires pigpio library, many thanks to the pigpio authors for their fantastic work.
 http://abyz.co.uk/rpi/pigpio/


 These names follow the MAX485 convention, strictly speaking only DE and DI are needed - RO can be ignored and RE can be pulled high (to 3.3v NOT 5)
 The pullup resistors need to be removed from the module as the MAX485 IC needs to be powered from 5v but will accept 3v logic.
 For best results add "disable_pvt=1" to /boot/config.txt, if this is missing the DMX generated will be corrupted periodically (flashes)
*/

// Pins, DE should be GPIO 21 if using a version 1 RPI board
#define RO			4						// RO (receiver output) data from the bus - ignored at the moment
#define RE			17						// -RE (receive enable), leave high at the moment					
#define DE			27						// DE (driver enable), idle low, high for transmit
#define DI			22						// DI (driver input) line for DMX data output

// Timings
#define BREAK_US		120
#define MAB_US			12
#define PREPACKET_IDLE_US	0
#define POSTPACKET_IDLE_US	50
#define SR			4						// symbol rate, 4us per bit

#define NUM_RAWBITS		6000						// Large enough to hold the DMX512 packet in bit form 
#define BSTEP			4

#include<stdio.h>
#include<stdlib.h>
#include<pigpio.h>
gpioPulse_t pulse[NUM_RAWBITS];
unsigned char dmx_values[513];							// 0 ignored, light value, dmx channels are 1 to 512 inclusive
int dmx_values_arg[512];							// values passed from command line or flags, 0=off, 255=max bright, 400=Cycle channel
int gpio=DI;


int output_low(int *idx, int duration_us)
{
	pulse[*idx].gpioOn = (1<<gpio);
	pulse[*idx].gpioOff = 0;
	pulse[*idx].usDelay = duration_us;
	*idx=*idx+1;
}

int output_high(int *idx, int duration_us)
{
   	pulse[*idx].gpioOn = 0;
   	pulse[*idx].gpioOff = (1<<gpio);
	pulse[*idx].usDelay = duration_us;
	*idx=*idx+1;
}

// Output 8 bit plus two stop bits
int output_serialbyte(int *pidx, unsigned char c)
{
	int b=0;

	output_low(pidx,SR);							// start bit
	for (b=0;b<8;b++)							// light value, 8 bits
	{
		if (c & (1<<b) )
			output_high(pidx,SR);
		else	output_low(pidx,SR);
	}
	output_high(pidx,SR*2);							// two stop bits
}



// Build a bit pattern with a DMX packet in it.
int build_dmx_packet()
{
	int ch;
	int pidx;

	pidx=0;
#ifdef PREPACKET_IDLE_US
	output_high(&pidx,PREPACKET_IDLE_US);					// idle
#endif
	output_low(&pidx,BREAK_US);						// BREAK
	output_high(&pidx,MAB_US);						// MAB (Mark after break)
	output_serialbyte(&pidx,0);                                             // "Start code" always 0 plus 2 stop bits

	for (ch=1;ch<=512;ch++)							// for each DMX channel
		output_serialbyte(&pidx,dmx_values[ch]);			// output all DMX channels, 8 bit data plus 2 stop bits

	output_high(&pidx,POSTPACKET_IDLE_US);					// idle
	return(pidx);								// tell caller how many bits we generated
}


send_dmx_packet(int numbits)
{
	int wave_id=0;

   	gpioWaveAddGeneric(numbits, pulse);
	//printf("generated %d bits\n",numbits);

	gpioWrite_Bits_0_31_Set( (1<<DE) );					// DE high (enable TX)
   	wave_id = gpioWaveCreate();
   	gpioWaveTxSend(wave_id, PI_WAVE_MODE_ONE_SHOT);				// Send this bit pattern once via DMA IO
	
	while (gpioWaveTxBusy()==1)						// Poll for DMA IO to complete
		usleep(20000);							// yield to kernel while DMA is in progress

	gpioWaveDelete(wave_id);						// finished with this pattern
	gpioWrite_Bits_0_31_Clear( (1<<DE) );					// DE low (disable TX)
}


int main(int argc, char *argv[])
{
	int i;
	int brightness=0;
	int numbits=0;
	int c,v;

   	if (gpioInitialise()<0) return -1;
   	gpioSetMode(RO, PI_INPUT);						// RO set as input, but we ignore it at the moment
   	gpioSetMode(RE, PI_OUTPUT);
   	gpioSetMode(DE, PI_OUTPUT);
   	gpioSetMode(DI, PI_OUTPUT);
	gpioWrite_Bits_0_31_Set( (1<<RE) );					// RE high
	gpioWrite_Bits_0_31_Clear( (1<<DE) );					// DE low (disable TX)

	for (i=1;i<=512;i++)							// for all dmx channels
		dmx_values_arg[i]=0;						// start with all channels off

	if (argc<2)
	{
		printf("Usage:\n");
		printf("rpi_dmaio_dmx_512_cycle_channels <CHANNEL>=<VALUE> where :\n");
		printf("<CHANNEL is DMX512 channel> and <VALUE> can be between 0=off and 255=on, 400=cycle this channel\n\n");
		printf("For example if a fixture has a master fade on channel 1 and green on channel 3 then:\n\n");
		printf("  rpi_dmaio_dmx_512_cycle_channels 1=255 3=400 would cycle the light in green\n\n");
		exit(0);
	}

	for (i=1;i<=argc-1;i++)
	{
		sscanf(argv[i],"%d=%d",&c,&v);					// string CHANNEL=VALUE
		//printf("c=%d  v=%d\n",c,v);
		if ( (c>=1) & (c<=512) )					// as long as DMX channel is valid 
		{
			if  ( ((v>=0) & (v<=255)) | (v==400) )			// as long as value is normal 8 bit or its 400
				dmx_values_arg[c]=v;
			else	
			{
				printf("ErrorValue %s ?\n",argv[i]);	
				exit(1);
			}
		}
		else	
		{
			printf("ErrorChannel %s ? \n",argv[i]);
			exit(1);
		}
	}

	printf("OK, setting lights, stepping in %d\n",BSTEP);
	fflush(stdout);
	brightness=0;
	while (1)
	{
		do
		{
			for (i=1;i<=512;i++)
			{
				if (dmx_values_arg[i]==400)			// special value for fading
					dmx_values[i]=brightness;		// then copy it across
				else	dmx_values[i]=dmx_values_arg[i];	// otherwise value is that user specified or default of 0
			}
			numbits=build_dmx_packet();				// put the bits into a packet
			send_dmx_packet(numbits);

			brightness=brightness+BSTEP;				// make brighter
			if (brightness<0)
				brightness=0;
			if (brightness>255)
				brightness=255;
		} while (brightness<255);

		do
		{
			for (i=1;i<=512;i++)
			{
				if (dmx_values_arg[i]==400)		
					dmx_values[i]=brightness;
				else	dmx_values[i]=dmx_values_arg[i];
			}
			numbits=build_dmx_packet();
			send_dmx_packet(numbits);

			brightness=brightness-BSTEP;				// make brighter
			if (brightness<0)
				brightness=0;
			if (brightness>255)
				brightness=255;
		} while (brightness>0);
	}
   	gpioTerminate();
}

