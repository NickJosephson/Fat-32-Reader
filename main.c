//
//  main.c
//  Fat 32 Reader
//
//  Created by Nicholas Josephson on 2018-07-07.
//

#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include "fat32.h"


// global variables
int fd; // fileIm
fat32BS bs; //boot sector
FSInfo fsInfo; //FS info


// function prototypes
void printInfo(void);
void printDirectoryTree(uint32_t clusterNum, int depth);
bool printFileAtPathFromCluster(char *path, uint32_t fromCluster);
void printFileAtCluster(uint32_t cluster, uint32_t fileSize);
uint32_t getSectorFromCluster(uint32_t clusterNum);
off_t getOffsetToSector(uint32_t sector);
off_t getOffsetToCluster(uint32_t cluster);
uint32_t getNextCluster(uint32_t currCluster);
void printDepth(int depth);
void printFileName(fat32Dir *dirEntry, int depth);
void printDirectoryName(fat32Dir *dirEntry, int depth);
int getNextSlashPos(char * path);
bool matchFileDirEntry(fat32Dir *dirEntry, char *path);
char *toUpperStr(char *str);
void verifyBootSector(void);
void verifyFSInfo(void);
void verifyFAT(void);

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Usage: fat32 <image path> <function> <argument>\n");
        return EXIT_FAILURE;
    }
    
    //attempt to opem image file
    const char *file_path = argv[1];
    fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("Couldn't open image: %s.\n", file_path);
        return EXIT_FAILURE;
    }
    
    //read in boot sector
    read(fd, &bs, sizeof(fat32BS));
    verifyBootSector();
    
    //read FSInfo
    lseek(fd, getOffsetToSector(bs.BPB_FSInfo), SEEK_SET);
    read(fd, &fsInfo, sizeof(FSInfo));
    verifyFSInfo();
    
    verifyFAT();
    
    //switch on function argument
    const char *function = argv[2];
    if (strcmp(function, "info") == 0) {
        printInfo();
    } else if (strcmp(function, "list") == 0) {
        printDirectoryTree(bs.BPB_RootClus, 0);
    } else if (strcmp(function, "get") == 0) {
        if (argc < 4) {
            printf("Usage: fat32 <image path> get <path to file>\n");
            close(fd);
            return EXIT_FAILURE;
        }
        char* path = strdup(argv[3]);
        if (!printFileAtPathFromCluster(toUpperStr(path), bs.BPB_RootClus)) {
            printf("File note found: %s\n", argv[3]);
        }
    } else {
        printf("Function not recognized: %s.\n", function);
        close(fd);
        return EXIT_FAILURE;
    }
    
    close(fd);
    return EXIT_SUCCESS;
}


// print information about the file system
void printInfo() {
    uint32_t FirstDataSector = bs.BPB_RsvdSecCnt + (bs.BPB_NumFATs * bs.BPB_FATSz32);
    uint32_t DataSec = bs.BPB_TotSec32 - FirstDataSector;
    uint32_t CountofClusters = DataSec / bs.BPB_SecPerClus;

    //Drive name
    printf("Volume Label: %.*s\n", BS_VolLab_LENGTH, bs.BS_VolLab);
    //Free space on the drive in kB
    printf("Free Space: %dkB\n", (fsInfo.FSI_Free_Count * bs.BPB_SecPerClus * bs.BPB_BytesPerSec)/1024);
    //The amount of usable storage on the drive (not free, but usable space)
    printf("Usable Space: %dkB\n", (CountofClusters * bs.BPB_SecPerClus * bs.BPB_BytesPerSec)/1024);
    //The cluster size in number of sectors, and in KB.
    printf("Cluster Size: %d sector(s) or %.2fKB\n", bs.BPB_SecPerClus , ((float)bs.BPB_SecPerClus * (float)bs.BPB_BytesPerSec)/1024.0);
}


// a recursive method to print the tree of a directory at a given cluster
void printDirectoryTree(uint32_t clusterNum, int depth) {
    off_t position = lseek(fd, getOffsetToCluster(clusterNum), SEEK_SET);
    uint32_t bytesReadInCurrCluster = 0;
    fat32Dir dirEntry;
    
    do {
        bytesReadInCurrCluster += sizeof(fat32Dir);
        position += read(fd, &dirEntry, sizeof(fat32Dir));

        uint8_t isHid = (*((uint8_t *) &dirEntry.DIR_Attr)) & DIR_ATTR_HIDDEN;
        uint8_t isDir = (*((uint8_t *) &dirEntry.DIR_Attr)) & DIR_ATTR_DIRECTORY;
        uint8_t isVol = (*((uint8_t *) &dirEntry.DIR_Attr)) & DIR_ATTR_VOLUME_ID;
        
        if (!isHid && !isVol && (uint8_t)dirEntry.DIR_Name[0] != DIR_EMPTY && (uint8_t)dirEntry.DIR_Name[0] != DIR_LAST) {
            if (isDir) {
                if (dirEntry.DIR_Name[0] != '.') {
                    printDirectoryName(&dirEntry, depth);

                    uint32_t subDirCluster = (dirEntry.DIR_FstClusHI << 16) | dirEntry.DIR_FstClusLO;
                    printDirectoryTree(subDirCluster, depth + 1);
                    lseek(fd, position, SEEK_SET); //restore position
                }
            } else {
                printFileName(&dirEntry, depth);
            }
        }

        if (bytesReadInCurrCluster >= bs.BPB_BytesPerSec * bs.BPB_SecPerClus) {
            bytesReadInCurrCluster = 0;
            clusterNum = getNextCluster(clusterNum);
            position = lseek(fd, getOffsetToCluster(clusterNum), SEEK_SET);
        }
    } while (dirEntry.DIR_Name[0] != DIR_LAST);
}


// recursive method to traverse directories then print the file at the given path from the given directory cluster
bool printFileAtPathFromCluster(char *path, uint32_t fromCluster) {
    uint32_t bytesReadInCurrCluster = 0;
    fat32Dir dirEntry;
    bool currDir = 0;
    
    //remove leading slash
    if (path[0] == '/') {
        path = &path[1];
    }
    
    int pos = getNextSlashPos(path);
    if (pos == -1) {
        currDir = true;
    } else {
        path[pos] = '\0';
    }
    
    lseek(fd, getOffsetToCluster(fromCluster), SEEK_SET);
    do {
        bytesReadInCurrCluster += sizeof(fat32Dir);
        read(fd, &dirEntry, sizeof(fat32Dir));
        
        uint8_t isHid = (*((uint8_t *) &dirEntry.DIR_Attr)) & DIR_ATTR_HIDDEN;
        uint8_t isDir = (*((uint8_t *) &dirEntry.DIR_Attr)) & DIR_ATTR_DIRECTORY;
        uint8_t isVol = (*((uint8_t *) &dirEntry.DIR_Attr)) & DIR_ATTR_VOLUME_ID;
        
        if (!currDir && isDir && !isHid && !isVol && (uint8_t)dirEntry.DIR_Name[0] != DIR_EMPTY && (uint8_t)dirEntry.DIR_Name[0] != DIR_LAST) {
            if (strcmp(strtok(dirEntry.DIR_Name, " "), path) == 0) {
                uint32_t subDirCluster = (dirEntry.DIR_FstClusHI << 16) | dirEntry.DIR_FstClusLO;
                return printFileAtPathFromCluster(&path[pos+1], subDirCluster);
            }
        } else if (!isDir && !isHid && !isVol && (uint8_t)dirEntry.DIR_Name[0] != DIR_EMPTY && (uint8_t)dirEntry.DIR_Name[0] != DIR_LAST) {
            if (matchFileDirEntry(&dirEntry, path)) {
                uint32_t fileCluster = (dirEntry.DIR_FstClusHI << 16) | dirEntry.DIR_FstClusLO;
                printFileAtCluster(fileCluster, dirEntry.DIR_FileSize);
                return true;
            }
        }
        
        if (bytesReadInCurrCluster > bs.BPB_BytesPerSec * bs.BPB_SecPerClus) {
            bytesReadInCurrCluster = 0;
            fromCluster = getNextCluster(fromCluster);
            lseek(fd, getOffsetToCluster(fromCluster), SEEK_SET);
        }
    } while (dirEntry.DIR_Name[0] != DIR_LAST);
    
    return false;
}


// print a file of a given size at the given cluster
void printFileAtCluster(uint32_t cluster, uint32_t fileSize) {
    uint32_t bytesReadInCurrCluster = 0;
    uint32_t bytesReadInTotal = 0;
    char currChar;
    
    lseek(fd, getOffsetToCluster(cluster), SEEK_SET);
    
    while (bytesReadInTotal < fileSize) {
        bytesReadInCurrCluster += 1;
        bytesReadInTotal += 1;
        
        read(fd, &currChar, 1);
        printf("%c", currChar);
        
        if (bytesReadInCurrCluster >= bs.BPB_BytesPerSec * bs.BPB_SecPerClus) {
            bytesReadInCurrCluster = 0;
            cluster = getNextCluster(cluster);
            lseek(fd, getOffsetToCluster(cluster), SEEK_SET);
        }
    }
    printf("\n");
}


// return the sector of the given cluster
uint32_t getSectorFromCluster(uint32_t clusterNum) {
    uint32_t FirstDataSector = bs.BPB_RsvdSecCnt + (bs.BPB_NumFATs * bs.BPB_FATSz32);
    return ((clusterNum - 2) * bs.BPB_SecPerClus) + FirstDataSector;
}


// return the byte (offset in file) of the given sector
off_t getOffsetToSector(uint32_t sector) {
    return sector * bs.BPB_BytesPerSec;
}


// return the byte (offset in file) of the given cluster
off_t getOffsetToCluster(uint32_t cluster) {
    return getSectorFromCluster(cluster) * bs.BPB_BytesPerSec;
}


// return the next cluster in the FAT for a given cluster
uint32_t getNextCluster(uint32_t currCluster) {
    uint32_t FATOffset = currCluster * 4;
    uint32_t ThisFATSecNum = bs.BPB_RsvdSecCnt + (FATOffset / bs.BPB_BytesPerSec);
    uint32_t ThisFATEntOffset = FATOffset % bs.BPB_BytesPerSec;
    
    lseek(fd, getOffsetToSector(ThisFATSecNum), SEEK_SET);
    lseek(fd, ThisFATEntOffset, SEEK_CUR);
    
    uint32_t value;
    read(fd, &value, sizeof(uint32_t));
    
    return (*((uint32_t *) &value)) & 0x0FFFFFFF;
}


// print a given amount of dashes for showing directory depth
void printDepth(int depth) {
    int i;
    for (i = 0; i < depth; i++) {
        printf("-");
    }
}


// print the name of a given directory entry as a file with a given depth
void printFileName(fat32Dir *dirEntry, int depth) {
    char start[9];
    char end[4];
    char *name = dirEntry->DIR_Name;
    
    int i;
    for (i = 0; i < 8 && name[i] != ' '; i++) {
        start[i] = name[i];
    }
    start[i] = '\0';
    
    for (i = 0; i < 3 && name[8 + i] != ' '; i++) {
        end[i] = name[8 + i];
    }
    end[i] = '\0';
    
    printDepth(depth);
    printf("File: %s.%s\n", start, end);
}


// print the name of a given directory entry as a directory with a given depth
void printDirectoryName(fat32Dir *dirEntry, int depth) {
    printDepth(depth);
    printf("DIR: %.*s\n", DIR_Name_LENGTH, dirEntry->DIR_Name);
}


// return the position of the next slash in a given string or -1
int getNextSlashPos(char * path) {
    int i = 0;
    while (path[i] != '\0') {
        if (path[i] == '/') {
            return i;
        }
        i += 1;
    }
    
    return -1;
}


// return whether a given name matched a given file directory entry
bool matchFileDirEntry(fat32Dir *dirEntry, char *path) {
    char newName[13];
    int startLength;
    char *name = dirEntry->DIR_Name;
    
    int i;
    for (i = 0; i < 8 && name[i] != ' '; i++) {
        newName[i] = name[i];
    }
    newName[i] = '.';
    startLength = i+1;
    
    for (i = 0; i < 3 && name[8 + i] != ' '; i++) {
        newName[startLength + i] = name[8 + i];
    }
    newName[startLength + i] = '\0';
    
    return strcmp(newName, path) == 0;
}


// convert a given string to all capital letters
char *toUpperStr(char *str) {
    int i = 0;
    while (str[i] != '\0') {
        str[i] = toupper(str[i]);
        i++;
    }
    return str;
}


// verify the boot sector has the correct signatures
void verifyBootSector() {
    if (bs.BS_SigA != 0x55 || bs.BS_SigB != 0xAA) {
        printf("Could not verify the boot sector.\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
}


// verify the FS info has the correct signatures
void verifyFSInfo() {
    if (fsInfo.FSI_TrailSig != 0xAA550000) {
        printf("Could not verify the file system info sector.\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
}


// verify the FAT has the correct signatures
void verifyFAT() {
    uint8_t sig1, sig2;

    lseek(fd, getOffsetToSector(bs.BPB_RsvdSecCnt), SEEK_SET);
    read(fd, &sig1, 1);
    
    lseek(fd, 1, SEEK_CUR);
    read(fd, &sig2, 1);
    
    if (sig1 != 0xF8 || sig2 != 0xFF) {
        printf("Could not verify the FAT signature.\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
}


