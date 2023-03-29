#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define VERBOSE 0

#ifdef WINDOWS
#define strdup _strdup
#endif

/*
 * struct cached_printf_args
 *
 * This is used as the pointer type of the dynamically allocated
 * memory which holds a copy of variable arguments.  The struct
 * begins with a const char * which recieves a copy of the printf()
 * format string.
 *
 * The purpose of ending a struct with a zero-length array is to
 * allow the array name to be a symbol to the data which follows
 * that struct.  In this case, additional memory will always be
 * allocted to actually contain the variable args, and cached_printf_args->args
 * will name the start address of that additional buffer space.
 *
 */
struct cached_printf_args
{
    const char * fmt;
    char  args[0];
};


/*
 * copy_va_args -- Accepts a printf() format string and va_list
 *                 arguments.
 *
 *                 Advances the va_list pointer in *p_arg_src in
 *                 accord with the specification in the format string.
 *
 *                 If arg_dest provided is not NULL, each argument
 *                 is copied from *p_arg_src to arg_dest according
 *                 to the format string.
 *
 */
int copy_va_args(const char * fmt, va_list * p_arg_src, va_list arg_dest)
{
    const char * pch = fmt;

    int processing_format = 0;

    while (*pch)
    {
        if (processing_format)
        {
            switch (*pch)
            {
            //case '!': Could be legal in some implementations such as FormatMessage()
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '.':
            case '-':

                // All the above characters are legal between the % and the type-specifier.
                // As the have no effect for caching the arguments, here they are simply
                // ignored.
                break;

            case 'l':
            case 'I':
            case 'h':
                printf("Size prefixes not supported yet.\n");
                exit(1);

            case 'c':
            case 'C':
                // the char was promoted to int when passed through '...'
            case 'x':
            case 'X':
            case 'd':
            case 'i':
            case 'o':
            case 'u':
                if (arg_dest)
                {
                     *((int *)arg_dest) = va_arg(*p_arg_src, int);
                     va_arg(arg_dest, int);
                }
                else
                    va_arg(*p_arg_src, int);
#if VERBOSE
                printf("va_arg(int), ap = %08X, &fmt = %08X\n", *p_arg_src, &fmt);
#endif
                processing_format = 0;
                break;

            case 's':
            case 'S':
            case 'n':
            case 'p':
                if (arg_dest)
                {
                    *((char **)arg_dest) = va_arg(*p_arg_src, char *);
                    va_arg(arg_dest, char *);
                }
                else
                    va_arg(*p_arg_src, char *);
#if VERBOSE
                printf("va_arg(char *), ap = %08X, &fmt = %08X\n", *p_arg_src, &fmt);
#endif
                processing_format = 0;
                break;

            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
                if (arg_dest)
                {
                    *((double *)arg_dest) = va_arg(*p_arg_src, double);
                    va_arg(arg_dest, double);
                }
                else
                    va_arg(*p_arg_src, double);
#if VERBOSE
                printf("va_arg(double), ap = %08X, &fmt = %08X\n", *p_arg_src, &fmt);
#endif
                processing_format = 0;
                break;
            }
        }
        else if ('%' == *pch)
        {
            if (*(pch+1) == '%')
                pch ++;
            else
                processing_format = 1;
        }
        pch ++;
    }

    return 0;
}

/*
 * printf_later -- Accepts a printf() format string and variable
 *                 arguments.
 *
 *                 Returns NULL or a pointer to a struct which can
 *                 later be used with va_XXX() macros to retrieve
 *                 the cached arguments.
 *
 *                 Caller must free() the returned struct as well as
 *                 the fmt member within it.
 *
 */
struct cached_printf_args * printf_later(const char *fmt, ...)
{
    struct cached_printf_args * cache;
    va_list ap;
    va_list ap_dest;
    char * buf_begin, *buf_end;
    int buf_len;

    va_start(ap, fmt);
#if VERBOSE 
    printf("va_start, ap = %08X, &fmt = %08X\n", ap, &fmt);
#endif

    buf_begin = (char *)ap;

    // Make the 'copy' call with NULL destination.  This advances
    // the source point and allows us to calculate the required
    // cache buffer size.
    copy_va_args(fmt, &ap, NULL);

    buf_end = (char *)ap;

    va_end(ap);

    // Calculate the bytes required just for the arguments:
    buf_len = buf_end - buf_begin;

    if (buf_len)
    {
        // Add in the "header" bytes which will be used to fake
        // up the last non-variable argument.  A pointer to a
        // copy of the format string is needed anyway because
        // unpacking the arguments later requires that we remember
        // what type they are.
        buf_len += sizeof(struct cached_printf_args);

        cache = malloc(buf_len);
        if (cache)
        {
            memset(cache, 0, buf_len);
            va_start(ap, fmt);
            va_start(ap_dest, cache->fmt);

            // Actually copy the arguments from our stack to the buffer
            copy_va_args(fmt, &ap, ap_dest);

            va_end(ap);
            va_end(ap_dest);

            // Allocate a copy of the format string
            cache->fmt = strdup(fmt);

            // If failed to allocate the string, reverse allocations and
            // pointers
            if (!cache->fmt)
            {
                free(cache);
                cache = NULL;
            }
        }
    }

    return cache;
}

/*
 * free_printf_cache - frees the cache and any dynamic members
 *
 */
void free_printf_cache(struct cached_printf_args * cache)
{
    if (cache)
        free((char *)cache->fmt);
    free(cache);
}

/*
 * print_from_cache -- calls vprintf() with arguments stored in the
 *                     allocated argument cache
 *
 *
 * In order to compile on gcc, this function must be declared to
 * accept variable arguments.  Otherwise, use of the va_start()
 * macro is not allowed.  If additional arguments are passed to
 * this function, they will not be read.
 */
int print_from_cache(struct cached_printf_args * cache, ...)
{
    va_list arg;

    va_start(arg, cache->fmt);
    vprintf(cache->fmt, arg);
    va_end(arg);
}

int main(int argc, char *argv)
{
    struct cached_printf_args * cache;

    // Allocates a cache of the variable arguments and copy of the format string.
    cache = printf_later("All %d of these arguments will be %s fo%c later use, perhaps in %g seconds.", 10, "stored", 'r', 2.2);

    // Demonstrate the time-line with some commentary to the output.
    printf("This statement intervenes between creation of the cache and its journey to the display.\n"

    // THIS is the call which actually displays the output from the cached printf.
    print_from_cache(cache);

    // Don't forget to return dynamic memory to the free store
    free_printf_cache(cache);

    return 0;

}