#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "ringfifo.h"

static unsigned int wr = 0;
static unsigned int rd = 0;

typedef struct 
{
 char test[333];
 int indx;
}testData;

static void srandWrite(ringFifo* iFifo, char* vStr)
{
  srand(time(NULL));
  int count = rand()%77;
  int i=0;
  testData* iTmp = malloc(sizeof(testData));
  
  for(i=0; i<count; i++)
  {
    memset(iTmp, 0, sizeof(testData));
    sprintf(iTmp->test,"%s:%ld:%d,wr=%ld rd=%ld", vStr, time(NULL), i, iFifo->mWrx, iFifo->mRdx);
    iTmp->indx = i;
    pushToRingFifo(iFifo, iTmp, sizeof(testData));
    wr++;
    usleep(100*100);
  }
  free(iTmp);
}


static void srandRead(ringFifo* iFifo)
{
  srand(time(NULL));
  int count = rand()%333;
  int i=0;
  testData* iTmp = malloc(sizeof(testData));
  for(i=0; i<count; i++)
  {
    memset(iTmp, 0, sizeof(testData));
    if(0 != popFromRingFifo(iFifo,iTmp, sizeof(testData)))
    {
      printf("Indx = %d  content = %s\n", iTmp->indx,iTmp->test);
      rd ++;
      usleep(100*100);
    }else{
      printf("read data = %ld  write Data = %ld\n",iFifo->mRdx, iFifo->mWrx);
      free(iTmp);
      return ;
    }
  }
  free(iTmp);
}


int main(int argv, char* argc[])
{
  ringFifo* iFifo =  initRingFifo(sizeof(testData), 345);
  while(1)
  {
    srandWrite(iFifo, argc[1]);
    srandRead(iFifo);
  }
  destroyRingFifo(iFifo);
}


