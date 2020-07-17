#ifndef __KERNEL_FS_H__
#define __KERNEL_FS_H__

#ifdef __cplusplus

#include <sys/stat.h>
#include <string.h>

extern "C" {
#endif



#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
class FileSystemNode
{
public:
    virtual void stat(struct stat *stat) = 0;
    virtual KString name() = 0;
};
class FileSystemDriver
{

};
}  // namespace kernel
#endif  // __cplusplus


#endif  // __KERNEL_FS_H__
