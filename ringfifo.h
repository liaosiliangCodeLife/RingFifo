/**
 * @file ringfifo.h
 *
 *  ringfifo api header file
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
#ifndef __RINGFIFO_H__
#define __RINGFIFO_H__

#define ringFifoErr            printf//debug interface
#define ringFifoDug            printf

#define RINGDATASIZE(_F)               ((((_F)->mWrx) - ((_F)->mRdx)))//data size caculate
#define RINGFREESIZE(_F)               (((_F)->mSize) - RINGDATASIZE(_F))//free space caculate
#define RINGFIFOWRPOS(_F)              ((((_F)->mWrx) % ((_F)->mSize)))//caculate write pointer address
#define RINGFIFORDPOS(_F)              ((((_F)->mRdx) % ((_F)->mSize)))//caculate read pointer address
#define RINGFIFOTOTALSIZE(_F)          (((_F)->mStep) * ((_F)->mPow) * ((_F)->mCount))//caculate the ringfifo size

/*Ringfifo struct*/
typedef struct 
{
  unsigned char* mBbuf;//cache buffer
  unsigned int   mSize;//buffer size
  unsigned int   mStep;//user data struct size
  unsigned int   mCount;//user data struct number
  unsigned int   mPow;//enlarge times about mCount.
  unsigned int   mNeedShrink;//need shringk
  unsigned long mRdx;//write pos
  unsigned long mWrx;//read pos
}ringFifo;


int popFromRingFifo(ringFifo* vFifo,void* vBuf, unsigned int vBlockSize);

int pushToRingFifo(ringFifo* vFifo, void* vBuf, unsigned int vBlockSize);

ringFifo* initRingFifo(unsigned int vStepSize, unsigned int vCount);

ringFifo* checkRingFifoShrink(ringFifo* vFifo);

void destroyRingFifo(ringFifo* vFifo);
#endif
