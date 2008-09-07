#ifndef __disk_h__
#define __disk_h__

#include "io.h"

//void ReadSectorsCHS(char* buffer, int sectors, int controller, int drive, int cylinder, int head, int sector, int bps);
//void WriteSectorsCHS(char* buffer, int sectors, int controller, int drive, int cylinder, int head, int sector, int bps);

void ReadSectors(char* buffer, int sectors, int controller, int drive, int lba);
void WriteSectors(char* buffer, int sectors, int controller, int drive, int lba);

#endif
