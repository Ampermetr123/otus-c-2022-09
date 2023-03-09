#include "signal_helpers.h"
#include <signal.h>

static const int stop_signals[] = {
    // The SIGTERM signal is a generic signal used to cause program termination. Unlike SIGKILL,
    // this signal can be blocked, handled, and ignored. It is the normal way to politely ask a
    // program to terminate.
    SIGTERM,

    // The SIGINT (“program interrupt”) signal is sent when the user types the INTR character
    // (normally C-c).
    SIGINT,

    // The SIGQUIT signal is similar to SIGINT, except that it’s controlled by a different key—the
    // QUIT character, usually C-\—and produces a core dump when it terminates the process, just
    // like a program error signal. You can think of this as a program error condition “detected” by
    // the user.
    SIGQUIT};

static const int error_signals[] = {
    // This signal is generated when a program tries to read or write outside the memory that is
    // allocated for it, or to write memory that can only be read.
    SIGSEGV,

    // This signal indicates an error detected by the program itself and reported by calling abort.
    SIGABRT,

    // The SIGFPE signal reports a fatal arithmetic error. Although the name is derived from
    // “floating-point exception”, this signal actually covers all arithmetic errors, including
    // division by zero and overflow. If a program stores integer data in a location which is then
    // used in a floating-point operation, this often causes an “invalid operation” exception,
    // because the processor cannot recognize the data as a floating-point number.
    SIGFPE,

    // The name of this signal is derived from “illegal instruction”; it usually means your program
    // is
    // trying to execute garbage or a privileged instruction. Since the C compiler generates only
    // valid instructions, SIGILL typically indicates that the executable file is corrupted, or that
    // you are trying to execute data.
    SIGILL,

    // This signal is generated when an invalid pointer is dereferenced. Like SIGSEGV, this signal
    // is
    // typically the result of dereferencing an uninitialized pointer. The difference between the
    // two is that SIGSEGV indicates an invalid access to valid memory, while SIGBUS indicates an
    // access to an invalid address. In particular, SIGBUS signals often result from dereferencing a
    // misaligned pointer, such as referring to a four-word integer at an address not divisible by
    // four.
    SIGBUS,

    // Bad system call; that is to say, the instruction to trap to the operating system was
    // executed,
    // but the code number for the system call to perform was invalid.
    SIGSYS};


void set_stop_signal_handler(void (*cleanup)(int signo)) {
  for (size_t i = 0; i < sizeof(stop_signals) / sizeof(stop_signals[0]); i++) {
    signal(stop_signals[i], cleanup);
  }
}

void set_error_signal_handler(void (*cleanup)(int signo)) {
  for (size_t i = 0; i < sizeof(error_signals) / sizeof(error_signals[0]); i++) {
    signal(error_signals[i], cleanup);
  }
}
