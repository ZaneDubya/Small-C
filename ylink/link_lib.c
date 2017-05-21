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
    // we need to know the total size of dependLength to set this value.
    dependData[iData + 2] = 0;
  }
}

readDependancies(uint fd, uint length) {
  // fix final dependancy count:
  int lastDepend;
  lastDepend = dependCount - 1;
  dependLength = length;
  dependData[lastDepend * DEPEND_DATA_LENGTH + 2] = 
    (dependLength - dependData[lastDepend * DEPEND_DATA_LENGTH + 1]) / 2;
  // read module dependancy data:
  dependMods = allocvar(dependLength, 1);
  read(fd, dependMods, dependLength);
}

writeDependancies(uint outfd) {
  int i, j;
  for (i = 0; i < dependCount; i++) {
    int location, offset, count;
    int *modules;
    location = dependData[i * DEPEND_DATA_LENGTH + 0];
    offset = dependData[i * DEPEND_DATA_LENGTH + 1];
    count = dependData[i * DEPEND_DATA_LENGTH + 2];
    modules = dependMods + offset;
    fprintf(outfd, "\n      %x Loc=%x Off=%x Cnt=%x {", 
            i, location, offset, count);
    for (j = 0; j < count; j++) {
      if (j != 0) {
        fputs(", ", outfd);
      }
      puthexint(outfd, *modules++);
    }
    fputc('}', outfd);
  }
}

writeLibData(uint fd) {
  int i;
  fprintf(fd, "===Library Data===\n");
  /* This would write out dictionary names:
  fprintf(fd, "  DictNames: %u bytes\n", dictCount * DICT_NAME_LENGTH);
  for (i = 0; i < dictCount; i++) {
    char *name;
    name = getDictName(i);
    fprintf(fd, "    %u: %s\n", i, name);
  }*/
  fprintf(fd, "  DictData: %u bytes", dictCount * DICT_DATA_LENGTH * 2);
  for (i = 0; i < dictCount; i++) {
    uint nameOffset, hash, moduleOffset;
    char *name;
    nameOffset = dictData[i * DICT_DATA_LENGTH + 0];
    hash = dictData[i * DICT_DATA_LENGTH + 1];
    moduleOffset = dictData[i * DICT_DATA_LENGTH + 2];
    if (moduleOffset == 0) {
      fprintf(fd, "\n    %x %s", i, "--------");
    }
    else {
      name = dictNames + nameOffset;
      fprintf(fd, "\n    %x %s %x %x", i, name, hash, moduleOffset);
    }
  }
  /* This would write out the dependancy data table.
  fprintf(fd, "\n  DependData: %u bytes\n", dependCount * DEPEND_DATA_LENGTH * 2);
  for (i = 0; i < dependCount * DEPEND_DATA_LENGTH * 2; i++) {
    puthexch(fd, dependData[i]);
    if ((i % 32) == 31) {
      fputc('\n', fd);
    }
  }
  */
  /* This would write out the dependancy modules table.*/
  fprintf(fd, "\n  DependMods: %u bytes\n", dependLength);
  for (i = 0; i < dependLength; i++) {
    puthexch(fd, dependMods[i]);
    if ((i % 32) == 31) {
      fputc('\n', fd);
    }
  }
  
}

getDictName(uint index) {
  uint j;
  char *begin, *c;
  begin = c = dictNames;
  j = 0;
  while (1) {
    if (*c++ == 0) {
      if (j == index) {
        return begin;
      }
      else {
        j++;
        begin = c;
      }
    }
  }
  return begin;
}