#include <inc/lib.h>

uint32_t ticks;

void
Time(int argc, char **argv)
{	
	ticks = 0;
	while(true){
		ticks++;
	}
}

uint32_t
getTicks(){
	return ticks;
}
