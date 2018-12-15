#ifndef __RINGFIFO_H__
#define __RINGFIFO_H__


typedef struct 
{
  unsigned char* mBbuf;
  unsigned int   mSize;
  unsigned int   mStep;
  unsigned int   mCount;
  unsigned int   mPow;
  unsigned int   mNeedShrink;
  unsigned long mRdx;
  unsigned long mWrx;
}ringFifo;


int popFromRingFifo(ringFifo* vFifo,void* vBuf, unsigned int vBlockSize);
int pushToRingFifo(ringFifo* vFifo, void* vBuf, unsigned int vBlockSize);
ringFifo* initRingFifo(unsigned int vStepSize, unsigned int vCount);
ringFifo* checkRingFifoShrink(ringFifo* vFifo);
void destroyRingFifo(ringFifo* vFifo);
#endif
