#include "./p1commands.h"
#define LIST_LONG 1
#define LIST_RECR 2
#define LIST_NVRB 4

/*
  Sistemas Operativos
  Práctica 1
  Carlos Torres (carlos.torres)
  Daniel Sergio Vega (d.s.vega)
  Grupo 4.3
*/

struct strfiledata {
  char inodenum[1000],permissions[12],hlinksnum[10],user[200],group[200],size[10],date[20],name[200],linksto[200];
};

char TipoFichero (mode_t m){
    switch (m & S_IFMT) { /*and bit a bit con los bits de formato,0170000 */
        case S_IFSOCK: return 's'; /*socket */
        case S_IFLNK: return 'l'; /*symbolic link*/
        case S_IFREG: return '-'; /* fichero normal*/
        case S_IFBLK: return 'b'; /*block device*/
        case S_IFDIR: return 'd'; /*directorio */
        case S_IFCHR: return 'c'; /*char device*/
        case S_IFIFO: return 'p'; /*pipe*/
        default: return '?'; /*desconocido, no deberia aparecer*/
    }
}


char * ConvierteModo2 (mode_t m){
    static char permisos[12];
    strcpy (permisos,"---------- ");
    permisos[0]=TipoFichero(m);
    if (m&S_IRUSR) permisos[1]='r'; /*propietario*/
    if (m&S_IWUSR) permisos[2]='w';
    if (m&S_IXUSR) permisos[3]='x';
    if (m&S_IRGRP) permisos[4]='r'; /*grupo*/
    if (m&S_IWGRP) permisos[5]='w';
    if (m&S_IXGRP) permisos[6]='x';
    if (m&S_IROTH) permisos[7]='r'; /*resto*/
    if (m&S_IWOTH) permisos[8]='w';
    if (m&S_IXOTH) permisos[9]='x';
    if (m&S_ISUID) permisos[3]='s'; /*setuid, setgid y stickybit*/
    if (m&S_ISGID) permisos[6]='s';
    if (m&S_ISVTX) permisos[9]='t';
    return (permisos);
}


// Funciones para las tareas de cada comando de la shell


int reclisting(const char * path,unsigned int options,int reclevel);

int createdir(const char *path){
  if (path == NULL){
    reclisting(".", 0,0);
    return 0;
  }

  else { //path!=NULL. Will create a new directory
    if (!mkdir(path, S_IRWXU | S_IRWXG)){ //creates directory
        printf(" Directory %s has been created\n", path);
        return 0;
    }

    else{
        if (errno == EEXIST){
            printf(" Error: File or directory %s already exists\n", path); //file already exists
            return -1;
        }
        else{
            printf(" Error: Directory %s could not be created\n", path); //other errors
            return -1;
        }
    }//else creates a directory
  }//else lists "."
}

int createfile(const char *path){
  int fd = open(path, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); //system call to create the file
  if (fd == -1){
      if (errno == EEXIST){ //file exists
          printf(" Error: File %s already exists\n",path);
          return -1;
      }
      else{ //other errors
          perror(strerror(errno));
          return -1;
      }
  }//end if open fails

  else{ //open returns on success
      printf(" File %s created\n", path); //file created successfully
      close(fd); //closes the file after being created
      return 0;
  }

}

int crear(const char * trozos[], int ntrozos, struct extra_info *ex_inf){
  if (trozos[1]==NULL){ //if no file name is given
    reclisting(".", 0,0);
    return 0;
  }
    
  if (!strcmp(trozos[1],"-d")){ //creates a directory
    for (int i = 2; (i < ntrozos); i++) {
      if (createdir(trozos[i])==-1){
        return -1;
      }
    }
    return 0;
  }

  else if ((trozos[1][0] == '-') && (trozos[1][0] != 'd')){ //not valid option
      printf(" %s : unrecognised command option\n", trozos[1]);
      return -1;
  } //else if

  else //tries to create a file
  {
    for (int i = 1; (i < ntrozos); i++) {
      if (createfile(trozos[i])==-1){
        return -1;
      }
    }
    return 0;
  }
}

int recdelete(const char * path) {
  struct stat statbuf;
  int intstat = lstat(path, &statbuf);
  if (!intstat){
    char isdir = TipoFichero(statbuf.st_mode);
    if (isdir != 'd') {
      remove(path);
    }
    else {
      DIR * dirpointer = opendir(path);
      if (dirpointer == NULL) {
        printf("%s\n",strerror(errno));
        return -1;
      }
      struct dirent * contents = readdir(dirpointer);
      while (contents != NULL) {
        if (!((!strcmp(contents->d_name,".")) || (!strcmp(contents->d_name,"..")))) { //do nothing for "." and ".."
          char newpath[MAXLEN];
          sprintf(newpath,"%s/%s",path,contents->d_name);
          recdelete(newpath);
        }
        errno = 0;
        contents = readdir(dirpointer);
        if (errno != 0) return -1;
        remove(path);
      }//while
    }//else
  }//if (!intstat)
  else{
      printf(" File or directory ''%s'' does not exist\n", path);
      return -1;
  }
  return 0;
}

int borrar(const char * trozos[], int ntrozos, struct extra_info *ex_inf){
  if (trozos[1]==NULL){
    reclisting(".",0,0);
    return 0;
  }
  if (!strcmp(trozos[1], "-r")){ //if the -r flag is specified
      for (int i = 2; i < ntrozos; i++) { //this for loop allows for several directories/files to be deleted in one line
	    if ((recdelete(trozos[i]))==-1){ //recursive directory deleting
          return -1; //if one fails, the function stops and the shell goes back to main
        }
      }
      return 0;
  }
  else if ((trozos[1][0] == '-') && (trozos[1][1] != 'r')){ //not valid flag
      printf(" %s: unrecognised command option\n", trozos[1]);
      return -1;
  }
  else{ //no flag is specified
    for (int i = 1; i < ntrozos; i++) { //this for loop allows for several files to be deleted in one line
    if (remove(trozos[i]) == -1){
        printf(" Error: File %s could not be deleted\n", trozos[1]);
        return -1;
    }
    else{
        printf(" File %s deleted\n", trozos[1]);
    }
    }
    return 0;
  }
}

struct strfiledata * getInfo(const char *path){ //This function is a helper that gets all the stat info of a file and puts it in a struct "strfiledata"
  struct strfiledata * strfinfo = NULL; //Struct that will contain the file info formatted as "strings"
  struct stat finfo; //Data buffer for fstat
  //MALLOC
  strfinfo = (struct strfiledata *) malloc(sizeof(struct strfiledata));
  if (lstat(path,&finfo) < 0) {
    printf("%s -> %s\n",strerror(errno),path); //Syscall
    return NULL;
  }
  //INODE NUMBER
  sprintf(strfinfo->inodenum,"%lu",finfo.st_ino);
  //TYPE & PERMISSIONS
  sprintf(strfinfo->permissions,"%s",ConvierteModo2(finfo.st_mode));
  //HARD LINK NUMBER
  sprintf(strfinfo->hlinksnum,"%lu",finfo.st_nlink);
  //USER
  struct passwd * tempu = getpwuid(finfo.st_uid);
  //Error
  if (tempu == NULL) {
    printf("Error: %s\n",strerror(errno));
    return NULL;
  };
  sprintf(strfinfo->user,"%s",tempu->pw_name);
  //GROUP
  struct group * tempg = getgrgid(finfo.st_gid);
  //Error
  if (tempu == NULL) {
    printf("Error: %s\n",strerror(errno));
    return NULL;
  };
  sprintf(strfinfo->group,"%s",tempg->gr_name);
  //SIZE
  sprintf(strfinfo->size,"%lu",finfo.st_size);
  //DATE FORMAT
  struct tm * mtime = localtime(&finfo.st_mtime);
  strftime(strfinfo->date,20,"%b %d %H:%M",mtime);
  //FILE NAME: cuts the rest of the path
  int j=0;
  for (int i = 0; path[i] != '\0'; i++) if (path[i] == '/') j = i+1;
  sprintf(strfinfo->name,"%s",&path[j]); //File name
  //LINKS TO
  if (readlink(path,strfinfo->linksto,200) == -1) {
    if (errno != 22) printf("%s\n",strerror(errno));
    strfinfo->linksto[0] = '\0';
    };
  return strfinfo;
}

int info(const char * trozos[], int ntrozos, struct extra_info *ex_inf){
  for (int i = 1; i < ntrozos; i++) {
    struct strfiledata *  data = getInfo(trozos[i]);
    if (data == NULL) return -1;
    printf("%8s %s %2s %8s %8s %8s %s %8s",data->inodenum,data->permissions,data->hlinksnum,data->user,data->group,
           data->size,data->date,data->name);
    if (data->linksto[0] != '\0') printf(" -> %s",data->linksto);
    printf("\n");
    free(data);
  }
  return 0;
}

int reclisting(const char* path,unsigned int options,int reclevel) {
  // EXTRACCION DE DATOS DEL FICHERO
  struct strfiledata *  data = getInfo(path);
  if (data == NULL) return -1;
  if (options & 0x4 && (data->name[0] == '.')) {free(data);return 0;} // -v -> 0x4
  // FORMATO PARA LISTADO RECURSIVO
  char indent[reclevel + 1];
  char indentchar = '-';
     //Si es directorio se cambia el caracter de indentacion
  if (!(options & 0x8) && data->permissions[0] == 'd') indentchar = '*';
  for (int i = 0; i < reclevel;i++) indent[i] = indentchar;
  indent[reclevel] = '\0';
  // IMPRESION DE DATOS DEL FICHERO
  if (options & 0x1) { //-l
    printf("%s %8s %s %2s %8s %8s %8s %s %8s",indent,data->inodenum,data->permissions,data->hlinksnum,data->user,data->group,
             data->size,data->date,data->name);
    if (data->linksto[0] != '\0') printf(" -> %s",data->linksto);
    }
  else {
    printf("%s %12s %s",indent,data->name,data->size);
  }
  // RECURSIVIDAD
  if (!(0x8 & options) && data->permissions[0] == 'd') {
      //0x8 retiene el listado recursivo a un solo nivel
      //ya que se activa cuando se llama a reclisting de un directorio sin -r
    free(data); //Liberar el espacio ocupado por la informacion del padre
    printf(" :\n");
    reclevel += 3; //Desplazamiento en la salida por pantalla de los ficheros hijo
    //APERTURA DEL DIRECTORIO
    DIR * dirpointer = opendir(path);
    if (dirpointer == NULL) {
      printf("%s",strerror(errno)); printf("\n");
      return 0;
    };
    //LECTURA DEL DIRECTORIO
    struct dirent * contents = readdir(dirpointer);
    while (contents != NULL) {
      char filename[516];
      sprintf(filename,"%s/%s",path,contents->d_name);
      // VVV Si la entrada en el directorio es el mismo o su padre o la recursividad esta desactivada
      if (!(0x2 & options) || !strcmp(contents->d_name,".") || !strcmp(contents->d_name,"..")) {
        reclisting(filename,options | 0x8,reclevel);
        //Estas llamadas tienen la recursividad desactivada por 0x8
      }
      else {
        reclisting(filename,options,reclevel);
      }
      //AVANZAR EN LA TABLA
      errno = 0;
      contents = readdir(dirpointer);
      if (errno != 0) return -1;
    }
    closedir(dirpointer);
  }
  else free(data);
  printf("\n");
  return 0;
}

int listar(const char * trozos[], int ntrozos, struct extra_info *ex_inf) {
  unsigned int options = 0x0;
  int argstart = 1;
  for (int i = 1; i < ntrozos && i < 4; i++) {
    if (!strcmp(trozos[i],"-l")) {options = options | 0x1; argstart++;}
    if (!strcmp(trozos[i],"-r")) {options = options | 0x2; argstart++;}
    if (!strcmp(trozos[i],"-v")) {options = options | 0x4; argstart++;}
  }
  int ret = 0;
  if (argstart == ntrozos) {
    char current[MAXLEN];
    ret = reclisting(getcwd(current,MAXLEN),options,0);
  }
  else
    for (int i = argstart;i < ntrozos; i++) ret |= reclisting(trozos[i],options,0);
  return ret;
}

