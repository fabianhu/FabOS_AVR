#include "avr/io.h"

.global leaveISR
.func leaveISR
leaveISR:

reti
.endfunc
