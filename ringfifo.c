#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "ringfifo.h"

#define ringFifoMalloc(_M)                   malloc((_M))
#define ringFifoFree(_R)                     free((_R))
#define ringFifoCpy(_C1, _C2, _C3)           memcpy((_C1), (_C2), (_C3))
#define ringFiFoSet(_S1, _S2, _S3)           memset((_S1), (_S2), (_S3))

#define ringFifoErr            printf
#define ringFifoDug            printf

#define RINGDATASIZE(_F)               ((((_F)->mWrx) - ((_F)->mRdx)))
#define RINGFREESIZE(_F)               (((_F)->mSize) - RINGDATASIZE(_F))
#define RINGFIFONEEDLOOPBACK(_F, _B)   (((((_F)->mWrx)%((_F)->mSize)) + (_B)) > ((_F)->mSize))
#define RINGFIFOWRPOS(_F)              ((((_F)->mWrx) % ((_F)->mSize)))
#define RINGFIFORDPOS(_F)              ((((_F)->mRdx) % ((_F)->mSize)))
#define RINGREARFREESIZE(_F)           (((_F)->mSize) - (((_F)->mWrx)) % ((_F)->mSize))
#define RINGFIFOTOTALSIZE(_F)          (((_F)->mStep) * ((_F)->mPow) * ((_F)->mCount))

#define RINGFIFOSHRINKTIME                   1024

typedef struct{
char test[234];
int  indx;
}testData;


static int copytoringfifo(ringFifo* vFifo,const unsigned char* vBuf, const unsigned int vBlocksSize)
{
  int i = 0;
  for( i=0; i<vBlocksSize; i+=vFifo->mStep)
  {
    if(1 != vFifo->mStep)
    {
      ringFifoCpy(&vFifo->mBbuf[RINGFIFOWRPOS(vFifo)], vBuf, vFifo->mStep);
    }
    else
    {
      vFifo->mBbuf[RINGFIFOWRPOS(vFifo)] = *vBuf;
    }
    vFifo->mWrx += vFifo->mStep;
    vBuf += vFifo->mStep;
  }
  return vBlocksSize;
}



static int copytobuffer(ringFifo* vFifo, unsigned char* vBuf, const unsigned int vBlocksSize)
{
  int i = 0;
  for(i=0; i<vBlocksSize; i+=vFifo->mStep)
  {
    if(1 != vFifo->mStep)
    {
      ringFifoCpy( vBuf, &vFifo->mBbuf[RINGFIFORDPOS(vFifo)], vFifo->mStep);
    }
    else
    {
      *vBuf = vFifo->mBbuf[RINGFIFOWRPOS(vFifo)];
    }
    vFifo->mRdx += vFifo->mStep;
    vBuf += vFifo->mStep;
  }
  return vBlocksSize;
}



static ringFifo* enlargeringfifo(ringFifo* vFifo)
{
  unsigned char* iTmBuf = NULL;
  unsigned long long iLen = 0; 
  vFifo->mPow++;
  iTmBuf = malloc(RINGFIFOTOTALSIZE(vFifo));
  if(iTmBuf)
  {
    iLen = copytobuffer(vFifo, iTmBuf, RINGDATASIZE(vFifo));
    ringFifoFree(vFifo->mBbuf);
    vFifo->mBbuf = iTmBuf;
    vFifo->mSize = RINGFIFOTOTALSIZE(vFifo);
    vFifo->mWrx = iLen;
    vFifo->mRdx = 0;
    return vFifo;
  }
  else
  {
    ringFifoErr("[%s:%d] Malloc the memory Error!!!\n",__FUNCTION__, __LINE__);
    return NULL;
  }
}


ringFifo* initRingFifo(unsigned int vStepSize, unsigned int vCount)
{
  ringFifo* retFifo = ringFifoMalloc(sizeof(ringFifo));
   
   if(retFifo)
   {
     retFifo->mSize = vStepSize*vCount;
     retFifo->mBbuf = ringFifoMalloc(retFifo->mSize);
     if(retFifo->mBbuf)
     {
       retFifo->mStep = vStepSize;
       retFifo->mPow = 1;
       retFifo->mCount = vCount;
       retFifo->mRdx = 0;
       retFifo->mWrx = 0;
       return retFifo;
     }
     else
     {
       ringFifoErr("[%s:%d] Malloc the memory Error!!!\n",__FUNCTION__, __LINE__);
       return NULL;
     }
   }
   else
   {
     ringFifoErr("[%s:%d] Malloc the memory Error!!!\n",__FUNCTION__, __LINE__);
     return NULL;
   }
}


int pushToRingFifo(ringFifo* vFifo, void* vBuf, unsigned int vBlockSize)
{
   if(!(vBlockSize%vFifo->mStep))
   {
     if(RINGFREESIZE(vFifo) > vBlockSize)
     {
       return copytoringfifo(vFifo, vBuf, vBlockSize);
     }
     else
     {
       if(enlargeringfifo(vFifo))
       {
         return pushToRingFifo(vFifo, vBuf, vBlockSize);
       }
       else
       {
         return 0;
       }
     }
   }
   else
   {
     ringFifoErr("[%s:%d] vBlockSize Error vBlockSize%%vFifo->vStepSize = %d!!!\n",__FUNCTION__, __LINE__,vBlockSize%vFifo->mStep);
     return 0;
   }
}


int popFromRingFifo(ringFifo* vFifo,void* vBuf, unsigned int vBlockSize)
{
  if(!(vBlockSize%vFifo->mStep))
  {
    if(RINGDATASIZE(vFifo) > vBlockSize)
    {
      return copytobuffer(vFifo, vBuf, vBlockSize);
    }
    else
    {
      return copytobuffer(vFifo, vBuf, RINGDATASIZE(vFifo));
    }
  }
  else
  {
    ringFifoErr("[%s:%d] vBlockSize Error vBlockSize%%vFifo->vStepSize = %d!!!\n",__FUNCTION__, __LINE__, vBlockSize%vFifo->mStep);
    return 0;
  }
}

ringFifo* checkRingFifoShrink(ringFifo* vFifo)
{
  if(!RINGDATASIZE(vFifo) && (1 != vFifo->mPow))
  {
    vFifo->mNeedShrink++;
    if(RINGFIFOSHRINKTIME == vFifo->mNeedShrink)
    {
      vFifo->mPow = 1;
      vFifo->mRdx = 0;
      vFifo->mWrx = 0;
      vFifo->mSize = RINGFIFOTOTALSIZE(vFifo);
      ringFifoFree(vFifo->mBbuf);
      vFifo->mBbuf = malloc(RINGFIFOTOTALSIZE(vFifo));
      return vFifo;
    }
  }
  else
  {
    vFifo->mNeedShrink = 0;
    return vFifo;
  }
}

void destroyRingFifo(ringFifo* vFifo)
{
  ringFifoFree(vFifo->mBbuf);
  ringFifoFree(vFifo);
}

static unsigned int wr = 0;
static unsigned int rd = 0;

static void srandWrite(ringFifo* iFifo, char* vStr)
{
  srand(time(NULL));
  int count = rand()%77;
  int i=0;
  testData* iTmp = malloc(sizeof(testData));
  
  for(i=0; i<count; i++)
  {
    memset(iTmp, 0, sizeof(testData));
    sprintf(iTmp->test,"%s:%d:%d,wr=%ld rd=%ld", vStr, time(NULL), i, iFifo->mWrx, iFifo->mRdx);
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
}


