%define BytesPerSector 		BPB+11
%define SectorsPerCluster    	BPB+13
%define ReservedSectors      	BPB+14
%define NumFATs              	BPB+16
%define RootEntries          	BPB+17
%define TotalSectors16      	BPB+19
%define MediaDescriptor     	BPB+21
%define FATSectors16        	BPB+22
%define SectorsPerTrack     	BPB+24
%define NumHeads                BPB+26
%define HiddenSectors       	BPB+28
%define TotalSectors32       	BPB+32
%define FATSectors32		BPB+36
%define RootCluster		BPB+44


%define PF_PRESENT	1<<0
%define PF_WRITE	1<<1
%define PF_USER		1<<2
%define PF_WRITETHROUGH	1<<3
%define PF_CACHEDISABLE 1<<4
%define PF_ACCESSED	1<<5
%define PF_DIRTY	1<<6
%define PF_PAT		1<<7
%define PF_GLOBAL	1<<8
%define PF_AVAIL1	1<<9
%define PF_AVAIL2	1<<10
%define PF_AVAIL3	1<<11

%define PF_LOCKED	PF_AVAIL1