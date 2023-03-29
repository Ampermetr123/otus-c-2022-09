#if defined(TROLL_LEVEL_MASK) && defined(TROLL_LEVEL_NAME) && defined(TROLL_LEVEL_FORMAT)
#include  "troll.h"

#ifndef TROLL_HELPERS
#define TROLL_HELPERS
#define CAT2(a, b) a ## b
#define CAT(a, b) CAT2(a, b)
#define Q(x) #x
#define QUOTE(x) Q(x)
// // #define QUOTE1(seq) CAT("\"", seq)
//  #define QUOTE(seq) "\"" ## seq   
// //#define QUOTE(seq) CAT("\"", CAT(seq,"\""))
#define LOG_PRINT_FUNC_NAME(LEVEL_NAME) CAT(troll_, LEVEL_NAME)
#endif

#if (TROLL_LEVEL_MASK > TROLL_MAX_LEVELS)
#error "TROLL_LEVEL_NUM is too big and must be less then TROLL_MAX_LEVELS"
#endif



static inline void LOG_PRINT_FUNC_NAME(TROLL_LEVEL_NAME)(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    troll_print(TROLL_LEVEL_MASK, QUOTE(TROLL_LEVEL_NAME), QUOTE(TROLL_LEVEL_FORMAT), fmt, &args);
    va_end(args);
    
} 

#undef TROLL_LEVEL_MASK
#undef TROLL_LEVEL_NAME
#undef TROLL_LEVEL_FORMAT

#else
#error "You should define TROLL_LEVEL_MASK, TROLL_LEVEL_NAME and TROLL_LEVEL_FORMAT before include this file"
#endif