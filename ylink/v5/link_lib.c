/******************************************************************************
* ylink - the ypsilon linker (c) 2017 Zane Wagner. All rights reserved.
* link_lib.c
* Methods that handle library object files.
******************************************************************************/
#include "stdio.h"
#include "link.h"

extern char* line;

// Library Dictionary - contains quick look up information.
// Each DictData entry is u16 offset to name table, u16 hash, u16 module offset
uint dictCount, *dictData;
uint dictNamNext = 0;
byte *dictNames; // dependancy data table

// Dependancies - indicates which modules require other modules.
// Each DependData entry is u16 module offset, u16 dependancy offset, u16 dependancy count
uint dependCount, *dependData;
uint dependLength; // length of dependacy modules table
byte *dependMods; // dependancy modules table

// Read the Library Dictionary, which provides rapid searching for all publicly
// defined names using a two-level hashing scheme. Note: the library dictionary
// is located in the final blocks of a library file, after all the blocks
// containing the object files.
// * The first 37 bytes of a dictionary block are indexes to block entries
//   (multiply value by 2 to get offset to entry).
// * Byte 38 is an index to empty space in the block (multiply value by 2 to
//   get offset to the next available space. If byte 38 is 255, block is full.
// * Each entry is a length-prefixed string, followed by a two-byte LE index 
//   of the module in the library defining this string can be found.
do_dictionary(uint fd, uint dictOffset[], uint blockCount) {
  byte offsets[38];
  uint iBlock, iEntry, moduleLocation, nameLength;
  uint blockOffset[2];
  // fprintf(outfd, "    Library Block Count=%u (%u / %u)",
  //        blockCount, dictCount, DICT_BLOCK_CNT);
  for (iBlock = 0; iBlock < blockCount; iBlock++) {
    // advance to the dictionary block
    blockOffset[0] = iBlock * 512;
    blockOffset[1] = 0;
    offsetfd(fd, dictOffset, blockOffset);
    read(fd, offsets, 38);
    for (iEntry = 0; iEntry < 37; iEntry++) {
      // ignore empty records (offset == 0)
      if (offsets[iEntry] != 0) {
        blockOffset[0] = (iBlock * 512) + (offsets[iEntry] * 2);
        offsetfd(fd, dictOffset, blockOffset);
        nameLength = read_strpre(line, fd);
        moduleLocation = read_u16(fd);
        AddDictEntry(line, iBlock, iEntry, moduleLocation);
      }
    }
    blockOffset[0] = iBlock * 512;
    blockOffset[1] = 0;
    offsetfd(fd, dictOffset, blockOffset);
  }
  blockOffset[0] = blockCount * 512;
  offsetfd(fd, dictOffset, blockOffset);
}

AllocDictMemory(uint count) {
  dictCount = count;
  dictNames = AllocMem(dictCount * DICT_NAME_LENGTH, 1);
  dictData = AllocMem(dictCount * DICT_DATA_LENGTH, 2);
}

AllocDependancyMemory(uint count) {
  dependCount = count;
  dependData = AllocMem(dependCount * DEPEND_DATA_LENGTH, 2);
}

AddDictEntry(char *name, uint hash0, uint hash1, uint moduleLocation) {
  uint iData, i, nameTableMax;
  nameTableMax = dictCount * DICT_NAME_LENGTH;
  iData = (hash0 * DICT_BLOCK_CNT + hash1) * DICT_DATA_LENGTH;
  dictData[iData + 0] = AddName(name, dictNames, &dictNamNext, nameTableMax);
  dictData[iData + 1] = hash0 + (hash1 << 8);
  dictData[iData + 2] = moduleLocation;
}



// F2H Extended Dictionary
// The extended dictionary is optional and indicates dependencies between
// modules in the library. Versions of LIB earlier than 3.09 do not create an
// extended dictionary. The extended dictionary is placed at the end of the
// library. 
do_extdict(uint length, uint fd) {
  uint i, page, offset, count;
  count = read_u16(fd);
  length -= 2;
  AllocDependancyMemory(count);
  for (i = 0; i <= count; i++) {
    page = read_u16(fd);
    offset = read_u16(fd);
    offset -= (count + 1) * 4;
    addDependancy(i, page, offset);
    length -= 4;
  }
  // ignore final null record
  readDependancies(fd, length);
  // fprintf(outfd, "\n    Library Dependencies (%u Modules)", count);
  // writeDependancies(outfd);
  // fputc('\n', outfd);
  return;
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
  dependMods = AllocMem(dependLength, 1);
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
    fputs("\n    LibMod", outfd);
    write_x8(outfd, i);
    fprintf(outfd, "  Loc=%x Off=%x Dep={", location, offset);
    for (j = 0; j < count; j++) {
      if (j != 0) {
        fputs(", ", outfd);
      }
      write_x16(outfd, *modules++);
    }
    fputc('}', outfd);
  }
}

putLibDict(uint fd) {
  int i;
  fprintf(fd, "\n    DictData: %u bytes", dictCount * DICT_DATA_LENGTH * 2);
  for (i = 0; i < dictCount; i++) {
    uint name, hash, moduleOffset;
    name = dictData[i * DICT_DATA_LENGTH + 0];
    hash = dictData[i * DICT_DATA_LENGTH + 1];
    moduleOffset = dictData[i * DICT_DATA_LENGTH + 2];
    if (moduleOffset == 0) {
      fprintf(fd, "\n    %x %s", i, "--------");
    }
    else {
      fprintf(fd, "\n    %x %s %x module_page=%x", i, name, hash, moduleOffset);
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
