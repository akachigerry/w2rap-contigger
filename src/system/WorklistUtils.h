///////////////////////////////////////////////////////////////////////////////
//                   SOFTWARE COPYRIGHT NOTICE AGREEMENT                     //
//       This software and its documentation are copyright (2010) by the     //
//   Broad Institute.  All rights are reserved.  This software is supplied   //
//   without any warranty or guaranteed support whatsoever. The Broad        //
//   Institute is not responsible for its use, misuse, or functionality.     //
///////////////////////////////////////////////////////////////////////////////

/*
 * \file WorklistUtils.h
 * \author tsharpe
 * \date Feb 4, 2009
 *
 * \brief A couple of utility classes for the Worklist.
 */
#ifndef SYSTEM_WORKLISTUTILS_H_
#define SYSTEM_WORKLISTUTILS_H_


#include "system/LockedData.h"
#include "system/SpinLockedData.h"
#include <cstddef>
#include <iostream>
#include <pthread.h>
#include <set>
#include <vector>

class WorklistParameterizer
{
public:
    WorklistParameterizer( size_t nUnits, size_t bytesPerUnit,
                           size_t minBatchSize, unsigned nThreads );

    // compiler-supplied copying and destructor are OK

    size_t getBatchSize() const { return mBatchSize; }
    size_t getNBatches() const { return mNBatches; }
    unsigned getNThreads() const { return mNThreads; }

private:
    size_t mBatchSize;
    size_t mNBatches;
    unsigned mNThreads;
};

class Dotter : private SpinLockedData
{
public:
    Dotter( size_t nBatches = 0 )
    : mNBatches(nBatches), mBatch(0), mNDots(0) {}

    // compiler-supplied copying and destructor are OK

    size_t getNBatches() const { return mNBatches; }
    void setNBatches( size_t nBatches )
    { mNBatches = nBatches; mBatch = 0; mNDots = 0; }

    void batchDone()
    { SpinLocker locker(*this);
      char sep = 0;
      if ( mNBatches <= 100 || 100*mBatch/mNBatches < 100*(mBatch+1)/mNBatches )
      {
          std::cout << '.' << std::flush;
          if ( ++mNDots == 50 ) sep = '\n';
          else if ( !(mNDots%10) ) sep = ' ';
      }
      if ( (mBatch += 1) == mNBatches ) sep = '\n';
      if ( sep ) std::cout << sep << std::flush; }

private:
    size_t mNBatches;
    size_t mBatch;
    unsigned mNDots;
};

/// A class to track the state of the queue.
class QueueState : public LockedData
{
public:
    QueueState()
    : mSize(0), mDone(false), mCondNotEmpty(*this), mCondEmpty(*this) {}

    QueueState( QueueState const& )=delete;
    QueueState& operator=( QueueState const& )=delete;

    // The entire interface, except for the constructor and destructor, is private because
    // the state should only be checked when the mutex is locked. The QueueStateManipulator,
    // which is a friend, locks the mutex in its constructor.
private:
    void setDone() { mDone = true; }
    bool isDone() { return mDone; }

    size_t incSize() { return ++mSize; }
    size_t decSize() { return --mSize; }
    size_t getSize() { return mSize; }
    void setSize( size_t sz ) { mSize = sz; }

    size_t mSize;            // current size of the work queue
    bool mDone;              // nothing more will be added to the queue
    Condition mCondNotEmpty; // predicate "queue not empty"
    Condition mCondEmpty;    // predicate "queue is empty"
    friend class QueueStateManipulator;
};

/// A class to change the queue state.
/// Constructor locks the QueueState mutex.
/// Destructor unlocks and does any necessary condition signaling.
class QueueStateManipulator
{
public:
    QueueStateManipulator( QueueState& qs )
    : mLocker(qs), mQS(qs)
    {
        mSize = mQS.getSize();
        mDone = mQS.isDone();
    }

    ~QueueStateManipulator();

    void setDone() { mQS.setDone(); }
    bool isDone() { return mQS.isDone(); }

    size_t incSize() { return mQS.incSize(); }
    size_t decSize() { return mQS.decSize(); }
    size_t getSize() { return mQS.getSize(); }
    void setSize( size_t sz ) { mQS.setSize(sz); }

    void waitForEmpty()
    { while ( getSize() )
      { mLocker.wait(mQS.mCondEmpty);
        mSize = mQS.getSize();
        mDone = mQS.isDone(); } }

    void waitForWork()
    { while ( !getSize() && !isDone() )
      { mLocker.wait(mQS.mCondNotEmpty);
        mSize = mQS.getSize();
        mDone = mQS.isDone(); } }

private:
    QueueStateManipulator( QueueStateManipulator const& ); // unimplemented -- no copying
    QueueStateManipulator& operator=( QueueStateManipulator const& ); // unimplemented -- no copying

    Locker mLocker;
    QueueState& mQS;
    size_t mSize; // size of queue at time of lock acquisition
    bool mDone;   // state of done flag at time of lock acquisition
};

/// Class to manage a pool of threads
class ThreadPool : LockedData
{
public:
    ThreadPool( size_t nThreads, size_t threadStackSize,
                void* (*threadFunc)(void*),
                void* threadFuncArg, bool use_gomp_affinity = true );
    ~ThreadPool();

    size_t getNThreads() { return mNThreads; }

    void threadDeathMonitor( long howOftenToCheckInSecs,
                             bool reportRestarts = true );
    size_t findThreadIndex( pthread_t );

    void shutdown();

    void setThreadAffinity();	// if an openmp affinity was set, we set mAttr for the next cpu in the list
    void setNextAffinity(size_t next) { mNextAffinity = next; }

private:
    void monitorThreads();

    /// prints an error message and exits if errNo is not 0
    static void checkThreadOp( int errNo, char const* msg )
    { if ( errNo ) die(errNo,msg); }
    static void die( int errNo, char const* msg );

    static size_t validateNThreads( size_t nThreads );
    static void* threadFunc( void* ptr );

    void* (*mThreadFunc)(void*);
    void* mThreadFuncArg;
    size_t mNThreads;
    pthread_t* mThreads;
    pthread_t mMonitorThread;
    std::vector<int> mGompCpuAffinity;
    size_t mNextAffinity;
    pthread_attr_t mAttr;
    Condition mCondExit;    // predicate "queue is empty"
    long mMonitorInterval;
    bool mReportRestarts;
    static size_t const DEFAULT_THREAD_STACK_SIZE = 8ul*1024ul*1024ul;
};

#endif /* SYSTEM_WORKLISTUTILS_H_ */
