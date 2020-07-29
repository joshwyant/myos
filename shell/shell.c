#include "shell.h"

int main()
{
    cls();
    print("MyOS shell\n");
    while (1) process_yield(); // loop forever (while being polite to other processes)
    return 0;
}
