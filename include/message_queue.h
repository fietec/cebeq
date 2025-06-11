#ifndef _CBQMSGQ_H
#define _CBQMSGQ_H

#include <cebeq.h>

CBQLIB void msgq_init();
CBQLIB void msgq_destroy();
CBQLIB void msgq_push(const char* message);
CBQLIB int  msgq_pop(char* out, int max_len);

#endif //_CBQMSGQ_H