#ifndef _tasks_fifo_h_
#define _tasks_fifo_h_

#include <stdint.h>
#include CHPL_THREADS_H


//
// The FIFO implementation of tasking is a least-common-denominator
// version whose purpose is to minimize the work needed to get Chapel
// tasking working on top of some new threading layer.
//
// The threading layer only has to supply a small amount of support in
// the form of supplementary types and callback functions.  The
// complete list is:
//
// For mutexes
//   type(s)
//     threadlayer_mutex_t
//     threadlayer_mutex_p
//   functions
//     threadlayer_mutex_init()
//     threadlayer_mutex_new()
//     threadlayer_mutex_lock()
//     threadlayer_mutex_unlock()
//
// For thread management
//   type(s)
//     <none>
//   functions
//     threadlayer_thread_id()
//     threadlayer_thread_cancel()
//     threadlayer_thread_join()
//
// For sync variables
//   type(s)
//     threadlayer_sync_aux_t
//   functions
//     threadlayer_sync_suspend()
//     threadlayer_sync_awaken()
//     threadlayer_sync_init()
//     threadlayer_sync_destroy()
//
// For single variables
//   type(s)
//     threadlayer_single_aux_t
//   functions
//     threadlayer_single_suspend()
//     threadlayer_single_awaken()
//     threadlayer_single_init()
//     threadlayer_single_destroy()
//
// For task management
//   type(s)
//     <none>
//   functions
//     threadlayer_init()
//     threadlayer_thread_create()
//     threadlayer_pool_suspend()
//     threadlayer_pool_awaken()
//     threadlayer_get_thread_private_data()
//     threadlayer_set_thread_private_data()
//
// The types are declared in the threads-*.h file for each specific
// threading layer, and the callback functions are declared here.  The
// interfaces and requirements for these other types and callback
// functions are described elsewhere in this file.
//
// Although the above list may seem long, in practice many of the
// functions are quite simple, and with luck also easily extrapolated
// from what is done for other threading layers.  For an example of an
// implementation, see "pthreads" threading.
//


//
// Type (and default value) used to communicate task identifiers
// between C code and Chapel code in the runtime.
//
typedef uint64_t chpl_taskID_t;
#define chpl_nullTaskID 0


//
// Thread management
//
threadlayer_threadID_t threadlayer_thread_id(void);
void threadlayer_thread_cancel(threadlayer_threadID_t);
void threadlayer_thread_join(threadlayer_threadID_t);


//
// Sync variables
//
// The threading layer's threadlayer_sync_aux_t may include any
// additional members the layer needs to support the suspend/awaken
// callbacks efficiently.  The FIFO tasking code itself does not
// refer to this type or the tl_aux member at all.
//
typedef struct {
  volatile chpl_bool is_full;
  threadlayer_mutex_t lock;
  threadlayer_sync_aux_t tl_aux;
} chpl_sync_aux_t;


//
// Single variables
//
// The threading layer's threadlayer_single_aux_t may include any
// additional members the layer needs to support the suspend/awaken
// callbacks efficiently.  The FIFO tasking code itself does not
// refer to this type or the tl_aux member at all.
//
typedef struct {
  volatile chpl_bool is_full;
  threadlayer_mutex_t lock;
  threadlayer_single_aux_t tl_aux;
} chpl_single_aux_t;


// Tasks

//
// Handy services for threading layer callback functions.
//
// The FIFO tasking implementation also provides the following service
// routines that can be used by threading layer callback functions.
//

//
// Is the task pool empty?
//
chpl_bool chpl_pool_is_empty(void);


//
// The remaining declarations are all for callback functions to be
// provided by the threading layer.
//

//
// These are called once each, from CHPL_TASKING_INIT() and
// CHPL_TASKING_EXIT().
//
void threadlayer_init(void);
void threadlayer_exit(void);


//
// Mutexes
//
void threadlayer_mutex_init(threadlayer_mutex_p);
threadlayer_mutex_p threadlayer_mutex_new(void);
void threadlayer_mutex_lock(threadlayer_mutex_p);
void threadlayer_mutex_unlock(threadlayer_mutex_p);


//
// Sync variables
//
// The CHPL_SYNC_WAIT_{FULL,EMPTY}_AND_LOCK() functions should call
// threadlayer_sync_suspend() when a sync variable is not in the desired
// full/empty state.  The call will be made with the sync variable's
// mutex held.  (Thus, threadlayer_sync_suspend() can dependably tell
// that the desired state must be the opposite of the state it initially
// sees the variable in.)  It should return (with the mutex again held)
// as soon as it can once either the sync variable changes to the
// desired state, or (if the given deadline pointer is non-NULL) the
// deadline passes.  It can return also early, before either of these
// things occur, with no ill effects.  If a deadline is given and it
// does pass, then threadlayer_sync_suspend() must return true;
// otherwise false.
//
// The less the function can execute while waiting for the sync variable
// to change state, and the quicker it can un-suspend when the variable
// does change state, the better overall performance will be.  Obviously
// the sync variable's mutex must be unlocked while the routine waits
// for the variable to change state or the deadline to pass, or livelock
// may result.
//
// The CHPL_SYNC_MARK_AND_SIGNAL_{FULL,EMPTY}() functions will call
// threadlayer_sync_awaken() every time they are called, not just when
// they change the state of the sync variable.
//
// Threadlayer_sync_{init,destroy}() are called to initialize or
// destroy, respectively, the contents of the tl_aux member of the
// chpl_sync_aux_t for the specific threading layer.
//
chpl_bool threadlayer_sync_suspend(chpl_sync_aux_t *s,
                                   struct timeval *deadline);
void threadlayer_sync_awaken(chpl_sync_aux_t *s);
void threadlayer_sync_init(chpl_sync_aux_t *s);
void threadlayer_sync_destroy(chpl_sync_aux_t *s);


//
// Single variables
//
// Analogous to the sync case, the CHPL_SINGLE_WAIT_FULL() function
// calls threadlayer_single_suspend() when a single variable is not
// full.  The call will be made with the single variable's mutex held.
// It should return (with the mutex again held) as soon as it can once
// either the single variable becomes full, or (if the given deadline
// pointer is non-NULL) the deadline passes.  It can return also early,
// before either of these things occur, with no ill effects.  If a
// deadline is given and it does pass, then threadlayer_single_suspend()
// must return true; otherwise false.
//
// The less the function can execute while waiting for the single
// variable to become full, and the quicker it can un-suspend when the
// variable does become full, the better overall performance will be.
// Obviously the single variable's mutex must be unlocked while the
// routine waits for the variable to become full or the deadline to
// pass, or livelock may result.
//
// The CHPL_SINGLE_MARK_AND_SIGNAL_FULL() function will call
// threadlayer_single_awaken() every time it is called, not just when it
// fills the single variable.
//
// Threadlayer_single_{init,destroy}() are called to initialize or
// destroy, respectively, the contents of the tl_aux member of the
// chpl_single_aux_t for the specific threading layer.
//
chpl_bool threadlayer_single_suspend(chpl_single_aux_t *s,
                                     struct timeval *deadline);
void threadlayer_single_awaken(chpl_single_aux_t *s);
void threadlayer_single_init(chpl_single_aux_t *s);
void threadlayer_single_destroy(chpl_single_aux_t *s);


//
// Task management
//

//
// The interface for thread creation may need to be extended eventually
// to allow for specifying such things as stack sizes and/or locations.
//
int threadlayer_thread_create(threadlayer_threadID_t*, void*(*)(void*), void*);


//
// Threadlayer_pool_suspend() is called when a thread finds nothing in
// the pool of unclaimed tasks, and so has no work to do.  The call will
// be made with the pointed-to mutex held.  It should return (with the
// mutex again held) as soon as it can once either the task pool is no
// longer empty or (if the given deadline pointer is non-NULL) the
// deadline passes.  It can return also early, before either of these
// things occur, with no ill effects.  If a deadline is given and it
// does pass, then threadlayer_pool_suspend() must return true;
// otherwise false.
//
// The less the function can execute while waiting for the pool to
// become nonempty, and the quicker it can un-suspend when that happens,
// the better overall performance will be.
//
// The mutex passed to threadlayer_pool_suspend() is the one that
// provides mutual exclusion for changes to the task pool.  Allowing
// access to this mutex simplifies the implementation for certain
// threading layers, such as those based on pthreads condition
// variables.  However, it also introduces a complication in that it
// allows a threading layer to create deadlock or livelock situations if
// it is not careful.  Certainly the mutex must be unlocked while the
// routine waits for the task pool to fill or the deadline to pass, or
// livelock may result.
//
// Note the FIFO tasking implementation's chpl_pool_is_empty() function,
// which the suspend callback can use to tell when the pool becomes
// nonempty.
//
chpl_bool threadlayer_pool_suspend(threadlayer_mutex_t*, struct timeval*);
void threadlayer_pool_awaken(void);


//
// Thread private data
//
// These set and get a pointer to thread private data associated with
// each thread.  This is for the use of the FIFO tasking implementation
// itself.  If the threading layer also needs to store some data private
// to each thread, it must make other arrangements to do so.
//
void  threadlayer_set_thread_private_data(void*);
void* threadlayer_get_thread_private_data(void);

#endif
