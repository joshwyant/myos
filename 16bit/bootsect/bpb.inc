oemname                 db      "MY OS   "
biosParameterBlock:
bpbBytesPerSector       dw      512
bpbSectorsPerCluster    db      64
bpbReservedSectors      dw      6
bpbNumFATs              db      2
bpbRootEntries          dw      512
bpbTotalSectors16       dw      0
bpbMediaDescriptor      db      0F8h
bpbFATSectors16         dw      081h
bpbSectorsPerTrack      dw      63
bpbHeads                dw      255
bpbHiddenSectors        dd      63
bpbTotalSectors32       dd      2104452
fatinfo:
fatDriveNumber          db      80h
fatReserved             db      0
fatBootSig              db      29h
fatVolID                dd      0
fatVolumeLabel          db      "MY OS      "
fatFileSystem           db      "FAT16   "
