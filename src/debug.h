#ifndef DEBUG_H
#define DEBUG_H

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1 // Default to off if not defined
#endif

#if DEBUG_PRINT
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#endif // DEBUG_H