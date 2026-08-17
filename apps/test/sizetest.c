#include <exec/types.h>

#include <stdio.h>
#include <stdint.h>

_Static_assert(sizeof(ULONG) == 4, "ULONG is not 32 bits");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t is not 32 bits");
_Static_assert(sizeof(unsigned long) == 4, "unsigned long is not 32 bits");

#include <stdint.h>

typedef char uint32_is_ulong[
    __builtin_types_compatible_p(uint32_t, unsigned long) ? 1 : -1
];

int main(void)
{
    printf("sizeof(ULONG) = %lu\n", (unsigned long)sizeof(ULONG));
    printf("sizeof(LONG)  = %lu\n", (unsigned long)sizeof(LONG));
    printf("sizeof(UWORD) = %lu\n", (unsigned long)sizeof(UWORD));
    printf("sizeof(APTR)  = %lu\n", (unsigned long)sizeof(APTR));

    return 0;
}