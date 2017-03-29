#ifndef GUARD_OF_INITLOADING_WORKER_H
#define GUARD_OF_INITLOADING_WORKER_H

#include <ace/config.h>

#define WORKER_MAX_STEP 1

extern UWORD g_uwWorkerStep;

/**
 * @brief Creates worker subtask.
 * @return Non-zero on success, otherwise 0.
 */
UWORD workerCreate(void);

/**
 * @brief Destroys worker subtask in a clean way.
 */
void workerDestroy(void);

/**
 * @brief Frees all data allocated & loaded by worker.
 * Use it to cleanup after data is no longer needed.
 */
void workerCleanup(void);

#endif // GUARD_OF_INITLOADING_WORKER_H