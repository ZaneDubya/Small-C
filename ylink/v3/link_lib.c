#include "stdio.h"
#include "link.h"

byte isLibrary = 0, hasDependancy = 0, idxLibMod = 0;
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

// F0H  LIBHDR Library Header Record
do_libhdr(uint outfd, uint length, uint fd) {
  byte flags, nextRecord;
  uint omfOffset[2], dictOffset[2], blockCount, nextLength;
  fputs("LIBHDR ", outfd);
  dictOffset[0] = read_u16(fd);
  dictOffset[1] = read_u16(fd);
  blockCount = read_u16(fd);
  flags = read_u8(fd);
  fprintf(outfd, "DictOffset=%u+(%ux2^16) Blocks=%u Flags=%x\n", 
          dictOffset[0], dictOffset[1], blockCount, flags);
  allocDictMemory(blockCount * DICT_BLOCK_CNT);
  length -= 8;
  // rest of record is zeros.
  while (length-- > 0) {
    read_u8(fd);
  }
  read_u8(fd); // checksum. assume correct.
  isLibrary = 1;
  // seek to library data offset, read data, return to module data offset
  btell(fd, omfOffset);
  do_dictionary(outfd, fd, dictOffset, blockCount);
  putLibDict(outfd);
  nextRecord = read_u8(fd);
  if (nextRecord == LIBDEP) {
    hasDependancy = 1;
    nextLength = read_u16(fd);
    fputs("\nF2  LIBDEP", outfd);
    do_libdep(outfd, nextLength, fd);
  }
  else {
    printf("Error: No dependancy information");
    abort(1);
  }
  bseek(fd, omfOffset, 0);
  return;
}

// The remaining blocks in the library module compose the dictionary. The count
// of blocks in the dictionary is given in the library header. The dictionary
// provides rapid searching for a name using a two-level hashing scheme. The
// number of ductionary blocks must be a prime number, each block containing
// exactly 37 dictionary indexes. A block is 512 bytes long. The first 37 bytes
// are indexes to block entries (multiply value by 2 to get offset to entry).
// Byte 38 is an index to empty space in the block (multiply value by 2 to get
// offset to the next available space. If byte 38 is 255 the block is full.
// Each entry is a length-prefixed string, followed by a two-byte LE mdoule 
// number in which the module in the library defining this string can be found.
do_dictionary(uint outfd, uint fd, uint dictOffset[], uint blockCount) {
  byte offsets[38];
  uint iBlock, iEntry, moduleLocation, nameLength;
  uint blockOffset[2];
  fprintf(outfd, "    Library Block Count=%u (%u / %u)",
          blockCount, dictCount, DICT_BLOCK_CNT);
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
        nameLength = read_strp(line, fd);
        moduleLocation = read_u16(fd);
        addDictData(line, iBlock, iEntry, moduleLocation);
      }
    }
    blockOffset[0] = iBlock * 512;
    blockOffset[1] = 0;
    offsetfd(fd, dictOffset, blockOffset);
  }
  blockOffset[0] = blockCount * 512;
  offsetfd(fd, dictOffset, blockOffset);
}

// F1H Library End
do_libend(uint outfd, uint length, uint fd) {
  fprintf(outfd, "LIBEND Padding=%x", length);
  clearsilent(outfd, length, fd);
  if (!isLibrary) {
    fputs('\n    Error: not a library!', outfd);
    abort(1);
  }
  fputc('\n', outfd);
  // writeLibData(outfd);
  fclose(fd);
}

// F2H Extended Dictionary
// The extended dictionary is optional and indicates dependencies between
// modules in the library. Versions of LIB earlier than 3.09 do not create an
// extended dictionary. The extended dictionary is placed at the end of the
// library. 
do_libdep(uint outfd, uint length, uint fd) {
    uint i, page, offset, count;
    count = read_u16(fd);
    length -= 2;
    allocDependancyData(count);
    for (i = 0; i <= count; i++) {
        page = read_u16(fd);
        offset = read_u16(fd);
        offset -= (count + 1) * 4;
        addDependancy(i, page, offset);
        length -= 4;
    }
    // ignore final null record
    readDependancies(fd, length);
    fprintf(outfd, "\n    Library Dependencies (%u Modules)", count);
    writeDependancies(outfd);
    fputc('\n', outfd);
    return;
}

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
    fputs("\n    LibMod", outfd);
    puthexch(outfd, i);
    fprintf(outfd, "  Loc=%x Off=%x Dep={", location, offset);
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
  // int i;
  /* This would write out dictionary names:
  fprintf(fd, "  DictNames: %u bytes\n", dictCount * DICT_NAME_LENGTH);
  for (i = 0; i < dictCount; i++) {
    char *name;
    name = getDictName(i);
    fprintf(fd, "    %u: %s\n", i, name);
  }*/
  /*fprintf(fd, "  DictData: %u bytes", dictCount * DICT_DATA_LENGTH * 2);
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
      fprintf(fd, "\n    %x %s %x module_page=%x", i, name, hash, moduleOffset);
    }
  }*/
  // This would write out the dependancy data table.
  /*fprintf(fd, "\n  DependData: %u bytes\n", dependCount * DEPEND_DATA_LENGTH * 2);
  for (i = 0; i < dependCount * DEPEND_DATA_LENGTH * 2; i++) {
    puthexch(fd, dependData[i]);
    if ((i % 32) == 31) {
      fputc('\n', fd);
    }
  }*/
  
  /* This would write out the dependancy modules table.*/
  /*fprintf(fd, "\n  DependMods: %u bytes\n", dependLength);
  for (i = 0; i < dependLength; i++) {
    puthexch(fd, dependMods[i]);
    if ((i % 32) == 31) {
      fputc('\n', fd);
    }
  }*/
}

putLibDict(uint fd) {
  int i;
  fprintf(fd, "\n    DictData: %u bytes", dictCount * DICT_DATA_LENGTH * 2);
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