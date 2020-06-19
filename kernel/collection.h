#ifndef __KERNEL_COLLECTION_H__
#define __KERNEL_COLLECTION_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CKeyInfo {
  unsigned key_size;
  int (*hash_fn)(const void *key);
  int (*eq_fn)(const void *key_a, const void *key_b);
} CKeyInfo;

typedef struct CRelationalKeyInfo {
  CKeyInfo *key_info;
  int (*compare_fn)(const void *a, const void *b);
} CRelationalKeyInfo;

struct CKeyValuePair {
  void *key;
  void *value;
} CKeyValuePair;

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
class KeyInfo
{
};  // class KeyInfo

class RelationalKeyInfo
{

};  // class RelationalKeyInfo

class KeyValuePair
{

};  // class KeyValuePair
}  // namespace kernel
#endif  // __cplusplus
#endif  // __KERNEL_COLLECTION_H__
