#include "stdio.h"
#include "link.h"

byte isLibrary = 0, hasDependancy = 0;
// Library Dictionary - contains quick look up information.
// Each DictData entry is u16 offset to name table, u16 hash, u16 module offset
uint dictCount, *dictData;
uint dictNamNext = 0;
byte *dictNames;
// Dependancies - indicates which modules require other modules.
// Each DependData entry is u16 module offset, u16 dependancy offset, u16 dependancy count
uint dependCount, *dependData;
uint dependLength;
byte *dependMods;


allocDictMemory(uint count) {
    printf("Allocating %u blocks...", count);
    dictCount = count;
    dictNames = allocvar(dictCount * DICT_NAME_LENGTH, 1);
    dictData = allocvar(dictCount * DICT_DATA_LENGTH, 2);
}

allocDependancyData(uint count) {
    dependCount = count;
    dependData = allocvar(dependCount * DEPEND_DATA_LENGTH, 2);
}

addDictData(char *name, uint hash0, uint hash1, uint moduleLocation) {
    uint iData, i, nameTableMax;
    nameTableMax = dictCount * DICT_NAME_LENGTH;
    iData = (hash0 * DICT_BLOCK_CNT + hash1) * DICT_DATA_LENGTH;
    dictData[iData + 0] = dictNamNext;
    dictData[iData + 1] = hash0 + (hash1 << 8);
    dictData[iData + 2] = moduleLocation;
    while (*name != 0x00) {
        dictNames[dictNamNext++] = *name++;
        if (dictNamNext >= nameTableMax) {
            printf("Error: Exceeded name table length of %u", nameTableMax);
            abort(1);
        }
        if (*name == 0) {
            dictNames[dictNamNext++] = 0x00;
        }
    }
}

addDependancy(uint i, uint moduleLocation, uint offset) {
    uint iData, count;
    iData = i * DEPEND_DATA_LENGTH;
    dependData[iData + 0] = moduleLocation;
    dependData[iData + 1] = offset;
    if (i > 0) {
        count = dependData[iData - DEPEND_DATA_LENGTH + 1];
        count = (offset - count) / 2;
        dependData[iData - DEPEND_DATA_LENGTH + 2] = count;
    }
    if (i == dependCount - 1) {
        count = (dependLength - offset) / 2;
        dependData[iData + 2] = count;
    }
}

readDependancies(uint fd, uint length) {
    dependLength = length;
    dependMods = allocvar(dependLength, 1);
    read(fd, dependMods, dependLength);
}

writeLibData(uint fd) {
    write(fd, dictNames, dictCount * DICT_NAME_LENGTH);
    write(fd, dictData, dictCount * DICT_DATA_LENGTH * 2);
    write(fd, dependData, dependCount * DEPEND_DATA_LENGTH * 2);
    write(fd, dependMods, dependLength);
}