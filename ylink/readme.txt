YLINK
Copyright 2017 Zane Wagner
All rights reserved.

YLINK will link together object files created by the Small Assembler, as well
as the Small-C Library file, and generate executable files that run in DOS. It 
only supports the 16-bit "small" memory model, where the Data and Stack 
segments share memory space, and the Code and Data/Stack segments must fit 
within 64kb.

YLINK expects that the first parameter will be a list of object and library
files, separated by the comma ',' character.

You may also use optional switches d, e, and l as follows:

  -d=xxx will output debug information to the file xxx. If this option is not
         used, no debug information will be created.
  -e=xxx will output the final exe or lib file to xxx. If this option is not
         used, the output file will be named out.exe.
  -l=xxx will take a list of files from file xxx, and output a library file.

Examples of invoking YLINK follow:

  YLINK a.obj,b.obj                         links a and b, outputs out.exe
  
  YLINK a.obj,b.obj,c.lib                   links a and b with library c.lib,
                                            outputs out.exe
                                            
  YLINK a.obj,b.obj,clib -e=a.exe           links a and b with library c.lib,
                                            outputs a.exe
                                            
  YLINK -l=lib.txt -e=clib.lib              concatenates all the object files
                                            listed in lib.txt, outputs library
                                            file clib.lib

Any number of input objects can be passed by listing them as parameters.
