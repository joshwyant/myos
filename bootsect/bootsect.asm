; Determine whether we are using FAT12, FAT16, or FAT32.
; FIXME: Preprocesser treats i.e. FAT16 as a constant instead of string in the error.
%ifdef FAT32
%ifdef FAT12
%error FAT32 must not be defined with FAT12.
%elifdef FAT16
%error FAT32 must not be defined with FAT16.
%else
%include "fat32.asm"
%endif
%elifdef FAT12
%ifndef FAT16
%include "fat.asm"
%else
%error FAT12 cannot be defined with FAT16.
%endif
%elifdef FAT16
%ifndef FAT12
%include "fat.asm"
%else
%error FAT16 cannot be defined with FAT12.
%endif
%else
%error Either FAT12, FAT16, or FAT32 must be defined. Use -dFATxx at the command line.
%endif