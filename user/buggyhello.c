#include <inc/lib.h>

void
umain(int argc, char **argv)
{
  int i;
  int a,b;
  a=0;b=0;
  for(i = 0;i<50;i++){
    a+=i;
    b+=a;
  }
  cprintf("a:%d b:%d\n",a,b);
}
