# Fat 32 Reader

A C program that reads a FAT 32-formatted drive. Built based on [this specification](https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc).

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
