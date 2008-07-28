#include "klib.h"
#include "dictionary.h"

dict* dict_create()
{
    dict *d = kmalloc(sizeof(dict));
    d->count = 0;
    d->items = kmalloc(sizeof(dict_item)*4);
    return d;
}

static void dict_ensure_size(dict *d, int count)
{
    int c = (d->count+3)/4;
    if ((count+3)/4 == c) return;
    dict_item* dip = kmalloc(((count+3)/4)*sizeof(dict_item));
    kmemcpy(dip, d->items, sizeof(dict_item)*count);
    kfree(d->items);
    d->items = dip;
}

void dict_add(dict *d, const char* name, void* value, int size)
{
    dict_ensure_size(d, d->count+1);
    d->items[d->count].name = kmalloc(kstrlen(name)+1);
    d->items[d->count].value = kmalloc(size);
    kstrcpy(d->items[d->count].name, name);
    kmemcpy(d->items[d->count].value, value, size);
    d->count++;
}

int dict_index(dict *d, const char* name)
{
    int i;
    for (i = 0; i < d->count; i++)
        if (!kstrcmp(name, d->items[i].name)) return i;
    return -1;
}

void dict_destroy(dict *d)
{
    int i;
    for (i = 0; i < d->count; i++)
    {
        kfree(d->items[i].name);
        kfree(d->items[i].value);
    }
    kfree(d->items);
    kfree(d);
}
