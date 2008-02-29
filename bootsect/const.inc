; DAP (Disk Access Packet)
; ========================
  %define Packet            200h
  %define PacketSize        200h  ; Size of packet structure
  %define PacketReserved1   201h
  %define PacketNumSectors  202h  ; Number of sectors to read
  %define PacketReseved2    203h
  %define PacketOffset      204h  ; Buffer offset
  %define PacketSegment     206h  ; Buffer segment
  %define PacketLBA0        208h  ; LO byte of LO word of LBA
  %define PacketLBA16       20Ah  ; HI byte of LO word of LBA
  %define PacketLBA32       20Ch  ; LO byte of HI word of LBA
  %define PacketLBA48       20Eh  ; HI byte of HI word of LBA

; ========================
; Program variables
; ========================
  %define FirstDataSector   210h  ; First data sector in FAT
  %define DriveNum          212h  ; Physical device number in DL register