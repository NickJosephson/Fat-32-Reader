#ifndef FAT32_H
#define FAT32_H

#include <inttypes.h>

/* FAT entry constants */
#define EOC 0x0FFFFFF8

/* boot sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8 

#pragma pack(push)
#pragma pack(1)
struct fat32BS_struct {
	char BS_jmpBoot[3];
	char BS_OEMName[BS_OEMName_LENGTH];
	uint16_t BPB_BytesPerSec; //bytes per sector
	uint8_t BPB_SecPerClus; //sectors per cluster
	uint16_t BPB_RsvdSecCnt; //count of sectors in reserved region
	uint8_t BPB_NumFATs; //number of FAT structures
	uint16_t BPB_RootEntCnt; //0, NA for FAT32
	uint16_t BPB_TotSec16; //0, NA for FAT32
	uint8_t BPB_Media; //NA for A4, media type (removable vs non-removable media)
	uint16_t BPB_FATSz16; //0, NA for FAT32
	uint16_t BPB_SecPerTrk; //NA for A4, relevant for media that have a geometry
	uint16_t BPB_NumHeads; //NA for A4, relevant for media that have a geometry
	uint32_t BPB_HiddSec; //NA for A4, count of hidden sectors preceding the partition that contains this FAT volume
	uint32_t BPB_TotSec32; //count of all sectors in all four regions of the volume
	uint32_t BPB_FATSz32; //count of sectors occupied by ONE FAT
	uint16_t BPB_ExtFlags; //NA for A4
	uint8_t BPB_FSVerLow; //NA for A4, version number of the FAT32 volume
	uint8_t BPB_FSVerHigh; //NA for A4, version number of the FAT32 volume
	uint32_t BPB_RootClus; //the cluster number of the first cluster of the root directory
	uint16_t BPB_FSInfo; //sector number of FSINFO structure in the reserved area
	uint16_t BPB_BkBootSec; //NA for A4, indicates the sector number in the reserved area of the volume of a copy of the boot record
	char BPB_reserved[12]; //NA for A4, reserved
	uint8_t BS_DrvNum; //NA for A4, drive number
	uint8_t BS_Reserved1; //NA for A4, reserved for WindowsNT
	uint8_t BS_BootSig; //NA for A4, extended boot signature, signature byte that indicates that the following three fields in the boot sector are present
	uint32_t BS_VolID; //NA for A4, volume serial number.
	char BS_VolLab[BS_VolLab_LENGTH]; //volume label, his field matches the 11-byte volume label recorded in the root directory
	char BS_FilSysType[BS_FilSysType_LENGTH]; //“FAT32 ”, NA for A4, file system name
	char BS_CodeReserved[420];
	uint8_t BS_SigA; //0x55, sector[510]
	uint8_t BS_SigB; //0xAA, sector[511]
};
#pragma pack(pop)

typedef struct fat32BS_struct fat32BS;


/* dir entry constants */
#define DIR_Name_LENGTH 11
#define DIR_EMPTY 0xE5
#define DIR_LAST 0x00
#define DIR_ATTR_HIDDEN 0x02
#define DIR_ATTR_VOLUME_ID 0x08
#define DIR_ATTR_DIRECTORY 0x10

#pragma pack(push)
#pragma pack(1)
struct fat32Dir_struct {
    char DIR_Name[DIR_Name_LENGTH];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
};
#pragma pack(pop)

typedef struct fat32Dir_struct fat32Dir;


#pragma pack(push)
#pragma pack(1)
struct FSInfo_struct {
    uint32_t FSI_LeadSig;
    char FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    char FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
};
#pragma pack(pop)

typedef struct FSInfo_struct FSInfo;

#endif
