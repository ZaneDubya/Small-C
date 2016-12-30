#include "stdio.h"
#include "link.h"

byte isLibrary = 0;
uint libDictOffset[2],  libDictSize;
uint libNameOffset = 0;
byte *libNames;
uint *libData;
uint modCount;
uint *modData;

allocDictMemory() {
    libNames = allocvar(libDictSize * 37 * DICT_NAME_LENGTH, 1);
    libData = allocvar(libDictSize * 37 * DICT_DATA_LENGTH, 2);
}

allocModMemory() {
    
}

addDictData(char *name, uint hash0, uint hash1, uint moduleLocation) {
    uint iData, i, nameTableMax;
    nameTableMax = libDictSize * DICT_BLOCK_CNT * DICT_NAME_LENGTH;
    iData = (hash0 * DICT_BLOCK_CNT + hash1) * DICT_DATA_LENGTH;
    libData[iData + 0] = libNameOffset;
    libData[iData + 1] = hash0 + (hash1 << 8);
    libData[iData + 2] = moduleLocation;
    while (*name != 0x00) {
        libNames[libNameOffset++] = *name++;
        if (libNameOffset >= nameTableMax) {
            printf("Error: Exceeded name table length of %u", nameTableMax);
            abort(1);
        }
        if (*name == 0) {
            libNames[libNameOffset++] = 0x00;
        }
    }
}

writeLibData(uint outfd) {
    write(outfd, libNames, libDictSize * 37 * DICT_NAME_LENGTH);
    write(outfd, libData, libDictSize * 37 * DICT_DATA_LENGTH * 2);
}