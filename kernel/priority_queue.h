#ifndef __KERNEL_PRIORITY_QUEUE__
#define __KERNEL_PRIORITY_QUEUE__

#include "vector.h"
#include "collection.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  PQ_MODE_INVALID,
  PQ_MODE_SMALLEST_FIRST,
  PQ_MODE_LARGEST_FIRST,
} PriorityQueueMode;

typedef struct kernel_priority_queue
{
    kernel_vector       *list;
    PriorityQueueMode mode;
    CRelationalKeyInfo *key_info;
    unsigned elem_count;
    unsigned elem_size;
} kernel_priority_queue;

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
class KPriorityQueue
{
public:
    KPriorityQueue() { /* TODO */ }
    ~KPriorityQueue() { /* TODO */ }
    /* TODO */
protected:
    kernel_priority_queue c_queue;
};      // class KPriorityQueue
}       // namespace kernel
#endif  // __cplusplus
#endif  // __KERNEL_PRIORITY_QUEUE__