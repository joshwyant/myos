#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* name;
    void* value;
} dict_item;

typedef struct {
    int count;
    dict_item *items;
} dict;

extern dict* dict_create();
extern void dict_add(dict *d, const char* name, void* value, int size);
extern int dict_index(dict *d, const char* name);
extern void dict_destroy(dict *d);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
