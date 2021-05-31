#ifndef __KERN_FS_FAT32BPB_H__
#define __KERN_FS_FAT32BPB_H__

#include "libs/types.h"
#include "mm/allocop.h"

#pragma pack(push, 1)

// Starting at offset 36 into the BPB, this is the structure for a FAT12/16 FS
typedef struct struct_BPBFAT1216_struct {
    uint8 BS_DriveNumber;        // 1
    uint8 BS_Reserved1;          // 1
    uint8 BS_BootSig;            // 1
    uint32 BS_VolumeID;          // 4
    uint8 BS_VolumeLabel[11];    // 11
    uint8 BS_FileSystemType[8];  // 8
} BPB1216_struct;

// Starting at offset 36 into the BPB, this is the structure for a FAT32 FS
typedef struct struct_BPBFAT32_struct {
    uint32 FATSize;              // 4
    uint16 ExtFlags;             // 2
    uint16 FSVersion;            // 2
    uint32 RootCluster;          // 4
    uint16 FSInfo;               // 2
    uint16 BkBootSec;            // 2
    uint8 Reserved[12];          // 12
    uint8 BS_DriveNumber;        // 1
    uint8 BS_Reserved1;          // 1
    uint8 BS_BootSig;            // 1
    uint32 BS_VolumeID;          // 4
    uint8 BS_VolumeLabel[11];    // 11
    uint8 BS_FileSystemType[8];  // 8
} BPB32_struct;

typedef struct struct_BPB_struct {
    uint8 BS_JumpBoot[3];        // 3
    uint8 BS_OEMName[8];         // 8
    uint16 BytesPerSector;       // 2
    uint8 SectorsPerCluster;     // 1
    uint16 ReservedSectorCount;  // 2
    uint8 NumFATs;               // 1
    uint16 RootEntryCount;       // 2
    uint16 TotalSectors16;       // 2
    uint8 Media;                 // 1
    uint16 FATSize16;            // 2
    uint16 SectorsPerTrack;      // 2
    uint16 NumberOfHeads;        // 2
    uint32 HiddenSectors;        // 4
    uint32 TotalSectors32;       // 4
  union {
    BPB1216_struct fat16;
    BPB32_struct fat32;
  } FSTypeSpecificData;
} BPB_struct;

typedef struct struct_FatFile83 {
  char filename[8];
  char extension[3];
  uint8 attributes;
  uint8 reserved;
  uint8 creationTimeMs;
  uint16 creationTime;
  uint16 creationDate;
  uint16 lastAccessTime;
  uint16 eaIndex;
  uint16 modifiedTime;
  uint16 modifiedDate;
  uint16 firstCluster;
  uint32 fileSize;
} FatFile83;

typedef struct struct_FatFileLFN {
  uint8 sequence_number;
  uint16 name1[5];      // 5 Chars of name (UTF 16???)
  uint8 attributes;     // Always 0x0f
  uint8 reserved;       // Always 0x00
  uint8 checksum;       // Checksum of DOS Filename.  See Docs.
  uint16 name2[6];      // 6 More chars of name (UTF-16)
  uint16 firstCluster;  // Always 0x0000
  uint16 name3[2];
} FatFileLFN;

typedef union struct_FatFileEntry {
  FatFile83 msdos;
  FatFileLFN lfn;
} FatFileEntry;

#pragma pack(pop)

// "Legacy" functions
uint32 fat_size(BPB_struct *bpb);
int total_sectors(BPB_struct *bpb);
int root_dir_sectors(BPB_struct *bpb);
int cluster_count(BPB_struct *bpb);
int fat_type(BPB_struct *bpb);
int first_data_sector(BPB_struct *bpb);
int first_sector_of_cluster(BPB_struct *bpb, int N);
int data_sectors(BPB_struct *bpb);
int fat_sector_number(BPB_struct *bpb, int N);
int fat_entry_offset(BPB_struct *bpb, int N);
int fat_entry_for_cluster(BPB_struct *bpb, uint8 *buffer, int N);

#endif
