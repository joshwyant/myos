jmp short start
nop
oemname                 db      "MYOS    "
biosParameterBlock:

%ifdef FAT12
; 4MB, 8Cyl, 16Heads, 63Spt, no MBR
bpbBytesPerSector       dw      512
bpbSectorsPerCluster    db      2
bpbReservedSectors      dw      1
bpbNumFATs              db      2
bpbRootEntries          dw      224
bpbTotalSectors16       dw      0
bpbMediaDescriptor      db      0F8h
bpbFATSectors16         dw      0xC
bpbSectorsPerTrack      dw      63
bpbHeads                dw      16
bpbHiddenSectors        dd      0
bpbTotalSectors32       dd      0x1F80

%elifdef FAT16
; 512MB, 1040Cyl, 16Heads, 63Spt, no MBR
bpbBytesPerSector       dw      512
bpbSectorsPerCluster    db      32
bpbReservedSectors      dw      1
bpbNumFATs              db      2
bpbRootEntries          dw      512
bpbTotalSectors16       dw      0
bpbMediaDescriptor      db      0F8h
bpbFATSectors16         dw      100h
bpbSectorsPerTrack      dw      63
bpbHeads                dw      16
bpbHiddenSectors        dd      0
bpbTotalSectors32       dd      0xFFF00

%elifdef FAT32
%if 0
; 1GB, 2080Cyl, 16Heads, 63Spt, no MBR
bpbBytesPerSector       dw      512
bpbSectorsPerCluster    db      8
bpbReservedSectors      dw      32
bpbNumFATs              db      2
bpbRootEntries          dw      0
bpbTotalSectors16       dw      0
bpbMediaDescriptor      db      0F8h
bpbFATSectors16         dw      0
bpbSectorsPerTrack      dw      63
bpbHeads                dw      16
bpbHiddenSectors        dd      0
bpbTotalSectors32       dd      0x1FFE00
%else
; ::: josh@josh-laptop 1GB /dev/sda3
bpbBytesPerSector       dw      512
bpbSectorsPerCluster    db      8
bpbReservedSectors      dw      32
bpbNumFATs              db      2
bpbRootEntries          dw      0
bpbTotalSectors16       dw      0
bpbMediaDescriptor      db      0F8h
bpbFATSectors16         dw      0
bpbSectorsPerTrack      dw      63
bpbHeads                dw      255
bpbHiddenSectors        dd      0x0301B744
bpbTotalSectors32       dd      0x201CC2
%endif
%endif

%ifdef FAT32
%if 0 ; 1GB, 2080Cyl, 16Heads, 63Spt, no MBR
bpbFATSectors32		dd	0x800
%else ; ::: josh@josh-laptop 1GB /dev/sda3
bpbFATSectors32		dd 	0x804
%endif
bpbExtFlags		dw	0
bpbFSVer		dw	0
bpbRootClus		dd	2
bpbFSInfo		dw	1
bpbBkBootSec		dw	6
bpbReserved    times 12 db	0
%endif

fatinfo:
fatDriveNumber          db      80h
fatReserved1            db      0
fatBootSig              db      29h
fatVolID                dd      0
fatVolumeLabel          db      "MYOS       "
%ifdef FAT12
fatFileSystem           db      "FAT12   "
%elifdef FAT16
fatFileSystem		db 	"FAT16   "
%else
fatFileSystem		db 	"FAT32   "
%endif
