/* 
**   AR -- File Archiver
**
**   usage: ar -{dptux} arcfile [file...]
**
**   Ar collects text files into a single archive file.
**   Files can be extracted, new ones added,
**   old ones replaced or deleted, and
**   a list of the archive contents produced.
**
**   The first argument is a switch from the following set.
**   The second argument is the name of the archive file.
**   The third and subsequent arguments are file names. 
**
**   -d  delete named files from the library.
**   -p  print named, or all, files on stdout.
**   -t  table of contents of named, or all, files to stdout.
**   -u  update the archive by adding/replacing files
**       (used to create a new library).
**       If no file names are specified in the command line,
**       they are obtained from stdin.
**   -x  extract named, or all, files.
**
**   Control-S to pause execution and control-C to abort.
**
**   This program was given as a class assignment in the
**   Computer Science Department at the University of Arizona.
**   It was contributed by Ernext Payne.  Orignially it was
**   written to work with tape archives, but this version has
**   been modified for higher speed operation with diskette
**   archives under CP/M.
*/

#include <stdio.h>

#define NAMESIZE 30
#define MAXLINE  500
#define MAXFILES 20
#define HDR      ">>>"
#define AUXSIZE  4096

char tname[]="  ar.$$$";
int fnptr[MAXFILES];
int fstat[MAXFILES];
int nfiles;
int errchk;

main(argc, argv) int argc, argv[]; {
  char cmd[3], aname[NAMESIZE];
  if(getarg(1,  cmd,       3,argc,argv) == EOF) usage();
  if(getarg(2,aname,NAMESIZE,argc,argv) == EOF) usage();
  if(aname[1] == ':') {
    tname[0] = aname[0];
    tname[1] = aname[1];
    }
  else left(tname);
  getfns(argc,argv);
  switch(toupper(cmd[1])) {
    case 'D': drop(aname);
              break;
    case 'T': table(aname);
              break;
    case 'U': update(aname);
              break;
    case 'X':
    case 'P': extract(aname, toupper(cmd[1]));
              break;
     default: usage();
    }
  }

/* acopy - copy size characters from fpi to fpo */
acopy(fpi,fpo,size) int fpi, fpo; int size; {
  int c;
  while(size-- > 0) {
    poll(YES);
    if((c = getc(fpi)) == EOF)
      break;
    putc(c,fpo);
    }
  }

/* addfile - add file "name" to archive */
addfile(name,fp) char *name; int fp; {
  int nfp;
  if((nfp = fopen(name,"r")) == NULL) {
    fprintf(stderr,"%s: can't open\n",name);
    errchk = 1;
    }
  if (errchk == 0) {
    if(name[1] == ':') name += 2;
    fprintf(fp,"%s %s %d\n",HDR,name,fsize(nfp));
    fcopy(nfp,fp);
    fclose(nfp);
    fprintf(stderr, " copied new %s\n", name);
    }
  }

/* amove - move file1 to file2 */
amove(file1,file2) char *file1, *file2; {
  if(errchk) {
    printf("fatal errors - archive not altered\n");
    unlink(file1);
    exit(7);
    }
  unlink(file2);
  if(file2[1] == ':') file2 += 2;
  if(rename(file1, file2)) {
    printf("can't rename %s to %s\n", file1, file2);
    exit(7);
    }
  }

/* cant - print file name and die */
cant(name) char *name; {
  fprintf(stderr,"%s: can't open\n",name);
  exit(7);
  }

/* drop - delete files from archive */
drop(aname) char *aname; {
  int afp, tfp;
  if(nfiles <= 0) /* protect innocents  */
    error("delete by name only");
  afp = mustopen(aname,"r");
  tfp = mustopen(tname,"w");
  auxbuf(tfp, AUXSIZE);
  replace(afp,tfp,'d');
  notfound();
  fclose(afp);
  fclose(tfp);
  amove(tname,aname);
  }

/* error - print message and die */
error(msg) char *msg; {
  fprintf(stderr,"%s\n",msg);
  exit(7);
  }

/* extract - extract files from archive */
extract(aname,cmd) char *aname, cmd; {
  int afp, efp;
  char ename[NAMESIZE], in[MAXLINE];
  int size;
  afp = mustopen(aname,"r");
  auxbuf(afp, AUXSIZE);
  if(cmd == 'P') efp = stdout;
  else           efp = NULL;
  while((size = gethdr(afp,in,ename)) >= 0)
    if(!fmatch(ename, YES)) fskip(afp,size);
    else {
      if(efp != stdout) efp = fopen(ename,"w");
      if(efp == NULL) {
        fprintf(stderr,"%s: can't create\n",ename);
        errchk = 1;
        fskip(afp,size);
        }
      else {
        if(cmd == 'P') {
          fprintf(efp, "\n컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�");
          fprintf(efp, "컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�\n");
          fprintf(efp, "                               %s", ename);
          fprintf(efp, "\n컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�");
          fprintf(efp, "컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�\n");
          }
        acopy(afp,efp,size);
        if(cmd == 'P') fprintf(stderr, "printed %s\n", ename);
        else           fprintf(stderr, "created %s\n", ename);
        if(efp != stdout) fclose(efp);
        }
      }
  notfound();
  fclose(afp);
  }

/* fcopy - copy file in to file out */
fcopy(in,out) int in, out; {
  int c;
  while((c = getc(in)) != EOF) {
    poll(YES);
    putc(c,out);
    }
  }

/* fmatch - check if name matches argument list */
fmatch(name, quit) char *name; int quit; {
  int i, done;
  char *fnp;
  if(nfiles <= 0) return(1);
  done = YES;
  for(i=0;i<nfiles;++i) {
    fnp = fnptr[i];
    if(fnp[1] == ':') fnp += 2;
    if(same(name,fnp) == 0) {
      fstat[i] = 1;
      return(1);
      }
    if(fstat[i] == 0) done = NO;
    }
  if(quit && done) exit(0);
  return(0);
  }

/* fsize - size of file in characters */
fsize(fp) int fp; {
  int i;
  for(i=0; getc(fp) != EOF; ++i) ;
  rewind(fp);
  return(i);
  }

/* fskip - skip n characters on file fp */
fskip(fp,n) int fp, n; {
  while(n-- > 0) {
    poll(YES);
    if(fgetc(fp) == EOF) break;
    }
  }

/* getfns - get file names into fname, check for duplicates */
getfns(argc,argv) int argc, argv[]; {
  int i, j;
  nfiles = argc - 3;
  if(nfiles > MAXFILES)
    error("too many file names");
  for(i=0,j=3; i<nfiles; i++,j++)
    fnptr[i] = argv[j];
  for(i = 0; i < nfiles-1; ++i)
    for(j = i+1; j < nfiles; ++j) {
      if(same(fnptr[i],fnptr[j]) == 0) {
        fprintf(stderr,"%s:duplicate file names\n",fnptr[i]);
        exit(7);
        }
    }
  }

/* gethdr - get header info from fp */
gethdr(fp,buf,name) int fp; char *buf, *name; {
  if(fgets(buf,MAXLINE,fp) == NULL)
    return(-1);
  buf = getwrd(buf,name);
  if(strcmp(name,HDR) != 0)
    error("archive not in proper format");
  buf = getwrd(buf,name);
  return(atoi(buf));
  }

/* getwrd - copy first word of s to t */
getwrd(s,t) char *s, *t; {
  while(isspace(*s)) ++s;
  while(*s != '\0' && !isspace(*s))  *t++ = *s++;
  *t = '\0';
  return(s);
  }

/* mustopen - open file or die */
mustopen(name,mode) char *name, *mode; {
  int fp;
  if(fp = fopen(name,mode)) return(fp);
  cant(name);
  }

/* notfound - print "not found" message */
notfound() {
  int i;
  for(i=0;i<nfiles;++i)
    if(fstat[i] == 0) {
      fprintf(stderr,"%s not in archive\n",fnptr[i]);
      errchk = 1;
    }
  }

/* replace - replace or delete files */
replace(afp,tfp,cmd) int afp, tfp; char cmd; {
  char in[MAXLINE], uname[NAMESIZE];
  int size;
  while((size = gethdr(afp,in,uname)) >= 0) {
    if(fmatch(uname, NO)) {
      if(cmd == 'u')    /* add new one */
        addfile(uname,tfp);
      fskip(afp,size);  /* discard old one */
      fprintf(stderr, "dropped old %s\n", uname);
      }
    else {
      fputs(in,tfp);
      acopy(afp,tfp,size);
      }
    }
  }

/* table - print table of archive contents  */
table(aname) char *aname; {
  char in[MAXLINE], lname[NAMESIZE];
  int afp, size;
  afp = mustopen(aname,"r");
  while((size = gethdr(afp,in,lname)) >= 0) {
    poll(YES);
    if(fmatch(lname, YES)) printf("%s\n", lname);
    fskip(afp,size);
    }
  notfound();
  fclose(afp);
  }

/* update - update existing files, add new ones at end */
update(aname) char *aname; {
  int afp, i, tfp;
  char fn[NAMESIZE];
  if((afp = fopen(aname,"r+")) == NULL)
    /* maybe archive does not exist yet */
    afp = mustopen(aname,"w+");
  tfp = mustopen(tname,"w+");
  auxbuf(tfp, AUXSIZE);
  replace(afp,tfp,'u');   /* update existing */
  if(nfiles > 0) {
    for(i=0;i<nfiles;++i) /* add new ones */
      if(fstat[i] == 0) {
        addfile(fnptr[i],tfp);
        if(errchk) break;
        fstat[i] = 1;
        }
    }
  else
    while(1) {
      poll(YES);
      if(iscons(stdin)) fprintf(stdout,"file - ");
      if(!fgets(fn, NAMESIZE, stdin) || *fn == '\n') break;
      for(i=0; fn[i]; i++)
        if((fn[i] = toupper(fn[i])) == '\n') fn[i] = NULL;
      addfile(&fn[0], tfp);
      if(errchk) break;
      }
  fclose(afp);
  fclose(tfp);
  amove(tname,aname);
  }

/* return <0,   0,  >0 according to s<t, s=t, s>t*/
same(s, t) char *s, *t; {
  while(toupper(*s) == toupper(*t)) {
    if(toupper(*s) == 0) return (0);
    ++s; ++t;
    }
  return (toupper(*s) - toupper(*t));
  }

/* usage - abort with a usage message */
usage() {
  error("usage: ar -{dptux} arcfile [file...]");
  }

