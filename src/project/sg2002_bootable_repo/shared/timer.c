#include "kraken.h"
void delay_cycles(volatile uint32_t n) { while (n--) cpu_relax(); }
