#include <vitasdk.h>

// Compatibility stub for shellUserMountById used by libcopyicons.a
// Returns a negative error code indicating not implemented on this environment.

int shellUserMountById(int id, const char *dev) {
    (void)id;
    (void)dev;
    return -1; // indicate failure
}