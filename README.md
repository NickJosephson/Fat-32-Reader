# Fat 32 Reader

A C program that reads a FAT 32-formatted drive.

## Usage
To compile this program, run make or execute the following from this directory:
```
clang main.c -o fat32
```
To run this program, execute the following from this directory:
```
./fat32 <image path> <function> <arguments>
```

To get image information:
```
./fat32 <image path> info
```

To output all files and directories on the image:
```
./fat32 <image path> list
```

To return a file on the image:
```
./fat32 <image path> get <path to file>
```
