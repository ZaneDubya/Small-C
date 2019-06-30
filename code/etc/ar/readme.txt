AR: The Source File Archive Utility

Usage: AR -{DPTUX} arcfile [file...]
       
       -{DPTUX}       Function switch.
       
       arcfile        Archive file specification.
       
       file...        List of individual filenames.
       
       
Description
       
       AR collects separate text files into a single archive file. Individual
       files  can  be  extracted  from  the archive, new ones added, old ones
       replaced or deleted, and  a  list  of  the  archive  contents  can  be
       produced.
       
            NOTE:  Although  the  archive is a pure ASCII file, you must
            not edit it because that would  change  the  length  if  its
            members  and  thereby  trick  AR  into  missing  the  member
            boundaries.
       
       The first argument must be a switch telling AR which of its  functions
       to  perform.  One and only one switch is required. The second argument
       must be the  name  of  an  archive  file.  The  third  and  subsequent
       arguments are filenames.
       
       While  AR  is  running, a control-S from the keyboard will cause it to
       pause execution and a control-C will cause  it  to  abort.  Press  the
       ENTER key to resume execution after a pause.
       
       
The Delete Switch
       
       The  -D  switch  commands  AR  to  delete  one  or more files from the
       archive. Filenames  appearing  after  the  archive  name  specify  the
       archive files to be deleted.
       
       
The Print Switch
       
       The -P switch commands AR to print one or more files from the archive.
       Filenames  appearing  after the archive name specify the archive files
       to be printed. If no names are given, all files  are  printed.  Output
       goes  to standard output file, and so may be redirected to any file or
       device.
       
       
The Table-of-Contents Switch
       
       The  -T  switch  commands  AR  to print the names of one or more files
       which the archive contains. Filenames appearing after the archive name
       specify the archive filenames to be listed. If no names are given, all
       filenames are listed. Output goes to standard output file, and so  may
       be redirected to any file or device.
       
       
The Update Switch
       
       The -U switch commands AR to update (add or replace) one or more files
       in the archive. Filenames appearing after the archive name specify the
       archive  files  to  be updated. If no names are given, names are taken
       from the standard input file. They may be entered (one per line)  from
       the keyboard or they may come from a redirection file.
       
       For  each name, AR looks for the designated file on disk and copies it
       into the archive file, replacing a preexisting file of the  same  name
       if necessary.
       
       This is the command used to create a new archive file.
       
       
The Extract Switch
       
       The -X switch commands AR to extract (copy) one or more files from the
       archive  to  disk.  Filenames appearing after the archive name specify
       the archive files to be copied. If no names are given, all  files  are
       copied. The contents of the archive are not affected by this command.
       
       
The Archive Filename
       
       An  archive  file must be named immediately after the function switch.
       It may contain a drive and path, but must specify the extension if the
       archive file has one. If an update function  is  being  performed  and
       there is no archive by the designated name, a new archive file will be
       created.
       
       
The Individual Filenames
       
       Filenames listed after arcfile designate which particular files within
       the  archive  are  subject  to  the  function  being  performed. These
       filenames must include their extensions, if they have extensions.