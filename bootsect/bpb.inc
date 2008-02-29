oemname                 db      "MY OS   "
biosParameterBlock:
bpbBytesPerSector       dw      512
bpbSectorsPerCluster    db      4
bpbReservedSectors      dw      4
bpbNumFATs              db      2
bpbRootEntries          dw      512
bpbTotalSectors16       dw      0
bpbMediaDescriptor      db      0F8h
bpbFATSectors16         dw      0EAh
bpbSectorsPerTrack      dw      63
bpbHeads                dw      255
bpbHiddenSectors        dd      0
bpbTotalSectors32       dd      3A8FEh
fatinfo:
fatDriveNumber          db      80h
fatReserved             db      0
fatBootSig              db      29h
fatVolID                dd      0
fatVolumeLabel          db      "MY OS      "
fatFileSystem           db      "FAT16   "