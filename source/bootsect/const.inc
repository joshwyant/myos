; DAP (Disk Access Packet)
; ========================
  %define Packet            7E00h
  %define PacketSize        7E00h  ; Size of packet structure
  %define PacketReserved1   7E01h
  %define PacketNumSectors  7E02h  ; Number of sectors to read
  %define PacketReseved2    7E03h
  %define PacketOffset      7E04h  ; Buffer offset
  %define PacketSegment     7E06h  ; Buffer segment
  %define PacketLBA0        7E08h  ; LO byte of LO word of LBA
  %define PacketLBA16       7E0Ah  ; HI byte of LO word of LBA
  %define PacketLBA32       7E0Ch  ; LO byte of HI word of LBA
  %define PacketLBA48       7E0Eh  ; HI byte of HI word of LBA

; ========================
; Program variables
; ========================
  %define FirstDataSector   7E10h  ; First data sector in FAT
  %define DriveNum          7E14h  ; Physical device number in DL register
  %define FATSector	    7E15h  ; Last FAT sector read