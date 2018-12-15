/**
 * @file ringfifo.c
 *
 *  ringfifo api source file
 *
 *
 * @author siliangLiao
 *
 * @date 7/6/2018
 *
 * @mail<liaosiliang1234@126.com>
 *
 * Copyright SiliangLiao. All right reserved.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "ringfifo.h"

/*macro define*/
#define ringFifoMalloc(_M)                   malloc((_M))
#define ringFifoFree(_R)                     free((_R))
#define ringFifoCpy(_C1, _C2, _C3)           memcpy((_C1), (_C2), (_C3))
#define ringFiFoSet(_S1, _S2, _S3)           memset((_S1), (_S2), (_S3))

#define RINGFIFOSHRINKTIME                   1024


/*
*Data copy to ringfifo
*
*@param in:
*    vFifo: ringfifo that you inited.
*    vBuf:  buffer that need restore to ringfifo.
*    vBlocksSize: the store data length
*
*@return :
*    the stored size of the data
*/
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


/*
*ringfifo copy to Data buffer
*
*@param in:
*    vFifo: ringfifo that you inited.
*    vBuf:  buffer that to store ringfifo data.
*    vBlocksSize: the store data length
*
*@return :
*    the stored size of the data
*/
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


/*
*ringfifo enlarge
*
*@param in:
*    vFifo: ringfifo that you inited.
*
*@return :
*    enlarge successed pointer of the ringfifo else NULL.
*/
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



/*
*Greate a ringfifo instance
*
*@param in:
*    vStepSize: The data struct size
*    vCount: number of the data struct
*
*@return :
*    init successed pointer of the ringfifo else NULL.
*/
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


/*
*Push date to ringfifo.
*
*@param in:
*    vFifo: ringfifo that you inited.
*    vBuf:  buffer that restore to ringfifo data.
*    vBlocksSize: the store data length
*
*@return :
*    the stored size of the data.
*/
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


/*
*Pop out from ringfifo.
*
*@param in:
*    vFifo: ringfifo that you inited.
*    vBuf:  buffer that to store ringfifo data.
*    vBlocksSize: the store data length
*
*@return :
*    the stored size of the data
*/
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


/*
*Simple check if ringfifo need shrink.
*
*@param in:
*    vFifo: ringfifo that you inited.
*
*@return :
*    the init ringfifo.
*/
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



/*
*Simple destory ringfifo.
*
*@param in:
*    vFifo: ringfifo that you inited.
*
*/
void destroyRingFifo(ringFifo* vFifo)
{
  ringFifoFree(vFifo->mBbuf);
  ringFifoFree(vFifo);
}
