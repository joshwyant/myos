#include "shell.h"
#include "../kernel/klib.h"

int main()
{
    cls();
    print("MyOS shell\n");
    while (1) ; // loop forever
    return 0;
}
