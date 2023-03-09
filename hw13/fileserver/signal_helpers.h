#pragma once

void set_stop_signal_handler(void (*cleanup)(int signo));

void set_error_signal_handler(void (*cleanup)(int signo));