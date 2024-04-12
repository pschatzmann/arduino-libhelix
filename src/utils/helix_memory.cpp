#include "Allocator.h"
#include "helix_log.h"

char log_buffer_helix[HELIX_LOG_SIZE];
libhelix::AllocatorExt alloc;

#ifdef __cplusplus
extern "C" {
#endif

void* helix_malloc(int size) { return alloc.allocate(size); }

void helix_free(void* ptr) { alloc.free(ptr); }

#ifdef __cplusplus
}
#endif
