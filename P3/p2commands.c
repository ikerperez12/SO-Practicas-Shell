#include "p2commands.h"
#define ASIGNAR_MALLOC 1
#define ASIGNAR_MMAP 2
#define ASIGNAR_C_SHARED 4
#define ASIGNAR_SHARED 8
#define ASIGNAR_ADDR 16
#define SS_ALL 0
#define SS_MALLOC 1
#define SS_MMAP 2
#define SS_SHM 4

//Funciones y estructuras para gestionar metadatos de memoria

struct melem {
  int mapfunc; //SS_MALLOC, SS_MMAP, SS_SHM
  void * dir;
  size_t size;
  char date[30];
  void * others;
};

struct immap {
  char filename[512];
  int fd;
};

//Constructs a new element and then inserts it into the polymorphic list
int buildElem(int mapf,unsigned long tam, void * where, iterator list, void * ex) {
  struct melem * new = malloc(sizeof(struct melem));
  //Error
  if (new == NULL) {
    perror("Error: BuildElem");
    return -1;
  }
  new->mapfunc = mapf;
  new->dir = where;
  new->size = tam;
  time_t now;
  if (time(&now)==-1){ //writes current time to NOW variable
    perror("Error BuildElem");
    return -1;
  };
  strftime(new->date,30,"%a %b %d %T %Y",localtime(&now));
  new->others = NULL;
  if (ex != NULL)
    new->others = ex;
  InsertElement(list,new);
  return 0;
}

//Prints all elements that match the requested flag (each flag corresponds to a mapping function)
int showElem(lista list, int flag) {
  struct melem * elem;
  for (iterator i = first(&list); !isLast(i); i = next(i)) {
    elem = getElement(i);
    if (elem->mapfunc == flag || flag == SS_ALL) {
      printf("%p: size: %lu ",(void *) elem->dir,elem->size);
      switch (elem->mapfunc) {
        case SS_MALLOC : {
          printf("malloc ");
          break;
        }
        case SS_MMAP : {
          struct immap * others = elem->others;
          printf("mmap %s (fd: %i) ",others->filename,others->fd);
          break;
        }
        case SS_SHM : {
          printf("shared memory (key: %i) ",*((int *) elem->others));
          break;
        }
        default : return -1;//Temp
      }
      printf("%s\n",elem->date);
    }
  }
  return 0;
}

//Looks for an element by the atribute that corresponds to the requested flag
//having this element the same mapping function that the flag specifies
int searchElem(lista list, int flag, struct melem ** e, void * id) {
  *e = NULL;
  struct melem * elem;
  for (iterator i = first(&list); !isLast(i); i = next(i)) {
    elem = getElement(i);
    if (elem->mapfunc == flag || flag == SS_ALL) {
      switch (flag) {
        case SS_MALLOC:
          if (elem->size == *((unsigned long *) id)) {
            *e = elem;
            return 0;
          }
          else break;
        case SS_MMAP: {
          struct immap * others = elem->others;
          if (!strcmp(others->filename,(char *)id)) {
            *e = elem;
            return 0;
          }
          break;
        }
        case SS_SHM:
          if (*(int *)elem->others == *(int *)id) {
            *e = elem;
            return 0;
          }
          else break;
        case SS_ALL:
          if (elem->dir == id) {
            *e = elem;
            return 0;
          }
          else break;
        default: return -1;
      }
    }
  }
  return 0;
}

//Frees the memory mapped referenced by the element using the corresponding call
//Also frees the memory used to store the element
//This function is given by parameter to the polymorphic list ADT for a clean removal of elements from the list
int freeMelem(void * elem) {
  struct melem * e = elem;
  char func[7];
  switch (e->mapfunc) {
  case SS_MALLOC: {
    sprintf(func,"malloc");
    free(e->dir);
    break;
  }
  case SS_MMAP: {
    if (munmap(e->dir,e->size)==-1){
      perror("Error: munmap at memory freeing");
      return -1;
    }
    sprintf(func,"mmap");
    free(e->others);
    break;
  }
  case SS_SHM: {
    if (shmdt(e->dir)==-1){
      perror("Error: shmdt at memory freeing");
      return -1;
    }
    sprintf(func,"shared");
    free(e->others);
    break;
  }
  default: return -1;
  }
  printf("Block at address %p deallocated (%s)\n",e->dir,func);
  free(elem);
  return 0;
}

void disposeMemory(struct extra_info * ex_inf) {
  disposeAll(&ex_inf->memoria,freeMelem);
}

//-Funciones-de-la-practica------------------------------------------------------

int asignar_malloc(const char * trozos[], int ntrozos, struct extra_info *ex_inf);
int asignar_mmap(const char * trozos[], int ntrozos, struct extra_info *ex_inf);
int asignar_crear_shared(const char * trozos[], int ntrozos, struct extra_info *ex_inf);
int asignar_shared(const char * trozos[], int ntrozos, struct extra_info *ex_inf);



int asignar(const char * trozos[], int ntrozos, struct extra_info *ex_inf){

  if (trozos[1] == NULL) return showElem(ex_inf->memoria,SS_ALL);

    if (!strcmp(trozos[1],"-malloc")) {
      return asignar_malloc(trozos,ntrozos,ex_inf);
    }
    if (!strcmp(trozos[1],"-mmap")) {
      return asignar_mmap(trozos,ntrozos,ex_inf);
    }
    if (!strcmp(trozos[1],"-crearshared")) {
      return asignar_crear_shared(trozos,ntrozos,ex_inf);
    }
    if (!strcmp(trozos[1],"-shared")) {
      return asignar_shared(trozos,ntrozos,ex_inf);
    }
    else{
      showElem(ex_inf->memoria,SS_ALL);
      return 0;
    }
}

int asignar_malloc(const char * trozos[], int ntrozos, struct extra_info *ex_inf){
    int tam;
    void *tmp;
    if (trozos[2] == NULL)
      return showElem(ex_inf->memoria,SS_MALLOC);
    tam = atoi(trozos[2]);
    tmp = malloc(tam);
    if (tmp==NULL){
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }
    printf("allocated %d at %p\n",tam,tmp);
    buildElem(SS_MALLOC,tam,tmp,&(ex_inf->memoria),NULL);
    return 0;
}

int asignar_mmap(const char * trozos[], int ntrozos, struct extra_info *ex_inf){
    unsigned int permisos_open = 0;
    unsigned int permisos_mmap = 0;
    struct stat statbuf;
    int fd = 0;
    void *file_ptr;
    const char *path = trozos[2];
    if (path==NULL){
      //Lista direcciones de memoria asignadas con mmap
      return showElem(ex_inf->memoria,SS_MMAP);
    }
    //lee los permisos pasados por parámetro
    if (trozos[3]==NULL){
        permisos_open = O_RDWR;
        permisos_mmap = PROT_READ | PROT_WRITE | PROT_EXEC;
    }

    else{
        for (int i = 0;trozos[3][i]!='\0';i++){
            if (trozos[3][i]=='r') (permisos_mmap |= PROT_READ);
            if (trozos[3][i]=='w') (permisos_mmap |= PROT_WRITE);
            if (trozos[3][i]=='x') (permisos_mmap |= PROT_EXEC);
        }
        //This assigns permisos_open the corresponding permission flag, depending on the mmap flags
        permisos_open = permisos_mmap & PROT_READ && permisos_mmap & PROT_WRITE ? O_RDWR :
                          permisos_mmap & PROT_WRITE ? O_WRONLY : O_RDONLY;
    }

    fd = open(path,permisos_open);
    if (fd==-1){
        perror("Error: open in asignar -mmap");
        return -1;
    }

    if (fstat(fd,&statbuf)==-1){
        perror("Error: fstat in asignar -mmap");
        return -1;
    }

    file_ptr = mmap(NULL,statbuf.st_size,permisos_mmap,MAP_PRIVATE,fd,0);
    if (file_ptr == MAP_FAILED){
        perror("Error: mmap in asignar -mmap");
        return -1;
    }

    printf("File %s mapped at %p\n",path,file_ptr);
    //
    struct immap * ifile = malloc(sizeof(struct immap));
    sprintf(ifile->filename,"%s",path);
    ifile->fd = fd;
    //Guarda la entrada en el historial de reservas de memoria
    buildElem(SS_MMAP,statbuf.st_size,file_ptr,&ex_inf->memoria,ifile);
    return 0;
}

int asignar_crear_shared(const char * trozos[], int ntrozos, struct extra_info *ex_inf){
    key_t key = 0;
    int size = 0,shared_id = 0;
    void *shm_ptr;
    if ((trozos[2]==NULL) || (trozos[3]==NULL)){ //no key or size are specified
      return showElem(ex_inf->memoria,SS_SHM);
    }
    key = (key_t) strtoul(trozos[2],NULL,10);
    size = atoi(trozos[3]);

    shared_id = shmget(key,size,IPC_CREAT|IPC_EXCL|0666);
    if (shared_id==-1){ //if shmget fails
        perror("Error: shmget in asignar -crearshared");
        return -1;
    }

    shm_ptr = shmat(shared_id,NULL,0);
    if (shm_ptr == MAP_FAILED){
        perror("Error: shmat in asignar -crearshared");
        return -1;
    }
    printf("Allocated shared memory (key %d) at %p\n",key,shm_ptr);
    int * keytoheap = malloc(sizeof(int));
    *keytoheap = key;
    buildElem(SS_SHM,size,shm_ptr,&ex_inf->memoria,keytoheap);
    return 0;
}

int asignar_shared(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
    int key = 0, shared_id = 0;
    void *shm_ptr;
    if (trozos[2]==NULL){ //no key is specified
      return showElem(ex_inf->memoria,SS_SHM);
    }
    key = atoi(trozos[2]);

    shared_id = shmget(key,0,0666);
    if (shared_id == -1){
        perror("Error: shmget in asignar -shared");
        return -1;
    }
    shm_ptr = shmat(shared_id,NULL,0);
    if (shm_ptr == MAP_FAILED){
        perror("Error: shmget in asignar -shared");
        return -1;
    }
    int * pkey = malloc(sizeof(int));
    *pkey = key;
    //
    struct shmid_ds * shstat = malloc(sizeof(struct shmid_ds));
    shmctl(shared_id,IPC_STAT,shstat);
    printf("Allocated shared memory (key %d) at %p\n",key,shm_ptr);
    //
    buildElem(SS_SHM,shstat->shm_segsz,shm_ptr,&ex_inf->memoria,pkey);
    free(shstat);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int desasignar_addr(const char * trozos[], int ntrozos, struct extra_info * ex_inf);
int desasignar_shared(const char * trozos[], int ntrozos, struct extra_info * ex_inf);
int desasignar_mmap(const char * trozos[], int ntrozos, struct extra_info * ex_inf);
int desasignar_malloc(const char * trozos[], int ntrozos, struct extra_info * ex_inf);


int desasignar(const char * trozos[], int ntrozos, struct extra_info *ex_inf){
    //No args
    if (trozos[1] == NULL) return showElem(ex_inf->memoria,SS_ALL);

    //Option selection:

    if (!strcmp(trozos[1],"-malloc")) {
      return desasignar_malloc(trozos,ntrozos,ex_inf);
    }
    if (!strcmp(trozos[1],"-mmap")) {
      return desasignar_mmap(trozos,ntrozos,ex_inf);
    }
    if (!strcmp(trozos[1],"-shared")) {
      return desasignar_shared(trozos,ntrozos,ex_inf);
    }
    else{
      char * endptr;
      strtoul(trozos[1],&endptr,16);
      if (*endptr != '\0'){
        fprintf(stderr, "Error desasignar: illegal argument \"%s\" (-malloc,-mmap,-shared, address([0x]HEXVALUE))\n",trozos[1]);
        return -1;
      }
      else{
        return desasignar_addr(trozos,ntrozos,ex_inf);
      }
    }
}

int desasignar_malloc(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
    unsigned long tam;
    if (trozos[2]==NULL){
      return showElem(ex_inf->memoria,SS_MALLOC);
    }
    tam = strtoul(trozos[2],NULL,10);
    //
    struct melem * e = NULL;
    searchElem(ex_inf->memoria,SS_MALLOC,&e,&tam);
    //
    if (e == NULL){
      return showElem(ex_inf->memoria,SS_MALLOC);
    }
    RemoveElement(&ex_inf->memoria,e,freeMelem);
    return 0;
}

int desasignar_mmap(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
    const char *path = trozos[2];
    if (trozos[2]==NULL){
      return showElem(ex_inf->memoria,SS_MMAP);
    }
    //buscar fichero fich en la lista de ficheros mapeados a memoria, sacar también el puntero a la direccion fileptr
    struct melem * e = NULL;
    searchElem(ex_inf->memoria,SS_MMAP,&e,(void *) path);//PENDIENTE CONST VOID
    //eliminar la entrada de la lista de archivos mapeados con mmap
    RemoveElement(&ex_inf->memoria,e,freeMelem);
    return 0;
}

int desasignar_shared(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
    int key = 0;
    if (trozos[2]==NULL){
      return showElem(ex_inf->memoria,SS_SHM);
    }
    key = atoi(trozos[2]);
    struct melem * e = NULL;
    searchElem(ex_inf->memoria,SS_SHM,&e,&key);
    //
    if (key==0){
      return showElem(ex_inf->memoria,SS_SHM);
    }
    //recuperar de la lista la información de la clave (key) y la dirección (ptr)
    RemoveElement(&ex_inf->memoria,e,freeMelem);
    return 0;
}

int desasignar_addr(const char *trozos[],int ntrozos, struct extra_info * ex_inf){
    void * addr = (void *) strtoul(trozos[1],NULL,16);
    //buscar la dirección addr en la lista
    struct melem * e = NULL;
    searchElem(ex_inf->memoria,SS_ALL,&e,addr);
    if (e != NULL) {
      RemoveElement(&ex_inf->memoria,e,freeMelem);
    }
    //Not found
    return showElem(ex_inf->memoria,SS_MALLOC)
      | showElem(ex_inf->memoria,SS_MMAP)
      | showElem(ex_inf->memoria,SS_SHM);
}

int borrarkey(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
    int shmid = 0;
    key_t key = 0;
    if (trozos[1]==NULL){
        printf("No key was entered\n");
        return -1;
    }
    key =(key_t) strtoul(trozos[1],NULL,10);
    if (key==IPC_PRIVATE){
      fprintf(stderr, "Error borrarkey: Invalid key\n");
      return -1;
    }
    shmid = shmget(key, 0, 0666); //0666 da permisos de lectura+escritura a todos los usuarios
    if (shmid==-1){
        perror("Error: shmget in borrarkey");
        return -1;
    }
    if (shmctl(shmid,IPC_RMID,NULL)==-1){
        perror("Error: shmctl in borrarkey");
        return -1;
    }
    printf("Shared memory region of key %d removed\n",key); //Nada es eliminado??
    return 0;
}

////

int cmd_mem(const char * trozos[], int ntrozos, struct extra_info * ex_inf) {
  if (trozos[1] == NULL) {
    int a,b,c;
    static int d,e,f;
    printf("Program functions' adresses: \n1: %p\n2: %p\n3: %p\n"
           "Extern variables' adresses: \n1: %p\n2: %p\n3: %p\n"
           "Automatic variables' adresses:\n1: %p\n2: %p\n3: %p\n",
           asignar,cmd_mem,recursiva,&d,&e,&f,&a,&b,&c);
    return 0;
  }
  else
    if (!strcmp(trozos[1],"-malloc")) return showElem(ex_inf->memoria,SS_MALLOC);
    else
      if (!strcmp(trozos[1],"-mmap")) return showElem(ex_inf->memoria,SS_MMAP);
      else
        if (!strcmp(trozos[1],"-shared")) return showElem(ex_inf->memoria,SS_SHM);
        else
          if (!strcmp(trozos[1],"-all")) return showElem(ex_inf->memoria,SS_ALL);
  //Not a valid argument:
  fprintf(stderr, "Error mem: illegal argument \"%s\" (-malloc,-mmap,-shared)\n",trozos[1]);
  return -1;
}

int volcar(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
  int cont = 25;
  const char *addr;

  if (trozos[1]==NULL){
    perror("Error volcar: Address not specified\n");
    return -1;
  }
  if (trozos[2]!=NULL) cont = atoi(trozos[2]);
  addr = (const char *) strtoul(trozos[1],NULL,16);

  unsigned char print[cont];
  for (int i = 0; i<cont; i+=25) {
    for (int j = 0; j < 25 && i+j < cont; j++) {
      print[j] = (unsigned char) *(addr+i+j);
      switch (print[j]){ //gestionar caracteres especiales
        case 10:
          printf(" LF "); //newline character
          break;
        case 13:
          printf(" CR "); //carriage return (used in MS Windows)
          break;
        case 32:
          printf(" SP "); //space character
          break;
        default:
          printf("  %c ",(isprint(*(addr+i+j)) ? *(addr+i+j) : ' '));
          break;
      }
    }
    printf("\n");
    for (int j = 0; j < 25 && i+j < cont; j++) {
      printf(" %2.2x ",print[j]);
    }
    printf("\n");
  }
  return 0;
}
/*
int volcar(char const * trozos[], int ntrozos, struct extra_info * ex_inf){
  int cont = 25;
  char const *addr;

  if (trozos[1]==NULL){
    perror("Error volcar: Address not specified\n");
    return -1;
  }
  if (trozos[2]!=NULL) cont = atoi(trozos[2]);
  addr = (char const *) strtoul(trozos[1],NULL,16);

  char print[25];
  for (int i = 0; i<cont; i+=25) {
    for (int j = 0; j < 25 && i+j < cont; j++) {
      print[j] = isprint(*(addr+i+j)) ? *(addr+i+j) : ' ';
      printf("  %c ",print[j]);
    }
    printf("\n");
    for (int j = 0; j < 25 && i+j < cont; j++) {
      printf(" %2.2x ",(unsigned char)*(addr+i+j));
    }
    printf("\n");
  }
  return 0;
}
*/
int llenar(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
  int cont = 128;
  char byte = 0x41;
  char* addr;

  if (trozos[1]==NULL){
    perror("Error llenar: Address not specified\n");
    return -1;
  }
  else addr = (char *)strtoul(trozos[1],NULL,16);
  if (trozos[2]!=NULL) cont = atoi(trozos[2]);
  if (trozos[3]!=NULL) byte = trozos[3][0];

  for(int i = 0;i<cont;i++) *(addr++) = byte;
  return 0;
}

void recursive_fun(int n) {
  char array[2048];
  static char static_array[2048];

  printf("\n");
  printf("Parameter n: %d en %p\n",n,&n);
  printf("Array in %p\n",array);
  printf("Static array in %p\n",static_array);
  printf("\n");

  if(--n > 0) recursive_fun(n);
}

int recursiva(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
  if (trozos[1]==NULL){
    fprintf(stderr, "Error recursiva: No parameter specified\n");
    return -1;
  }
  int num = atoi(trozos[1]);
  recursive_fun(num);
  return 0;
}

int rfich(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
  void *addr;
  int cont = -1;
  struct stat statbuf;
  int fd;
  if (trozos[1]==NULL){
    fprintf(stderr, "Error rfich: File not specified\n");
    return -1;
  }
  if (trozos[2]==NULL){
    fprintf(stderr, "Error rfich: Address not specified\n");
    return -1;
  }
  else addr = (void *)strtoul(trozos[2],NULL,16);
  if (trozos[3]!=NULL) cont = atoi(trozos[3]);
  fd = open(trozos[1],O_RDONLY);
  if (fd==-1){
    perror("Error: open in rfich");
    return -1;
  }
  if (cont==-1){
    if (stat(trozos[1],&statbuf) == -1) perror("Error: stat in rfich");
    else
      cont = statbuf.st_size;
  }
  if (read(fd,addr,cont)==-1){
    perror("Error: read in rfich");
    return -1;
  }
  return 0;
}

int wfich(const char * trozos[], int ntrozos, struct extra_info * ex_inf){
  char overwrite = 0;
  void* addr;
  int cont, fd;
  if (trozos[1] == NULL) {
    fprintf(stderr, "Error wfich: no arguments (wfich [-o] file address cont)\n");
    return -1;
  }
  if (!strcmp("-o",trozos[1])) overwrite=1;
  else
    if (trozos[1][0] == '-'){
      fprintf(stderr, "Error wfich: option %s unrecognised\n", trozos[1]);
      return -1;
    }

  if (trozos[1+overwrite]==NULL){
    fprintf(stderr, "Error wfich: File not specified\n");
    return -1;
  }
  if (trozos[2+overwrite]==NULL){
    fprintf(stderr, "Error wfich: Address not specified\n");
    return -1;
  }
  if (trozos[3+overwrite]==NULL){
    fprintf(stderr, "Error wfich: cont not specified\n");
    return -1;
  }
  addr = (void *) strtoul(trozos[2+overwrite],NULL,16);
  cont = atoi(trozos[3+overwrite]);
  if (overwrite){
    fd = open(trozos[1+overwrite],O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  }
  else{
    fd = open(trozos[1+overwrite],O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  }
  if (fd == -1) perror("Error");
  if (write(fd, addr, cont)==-1){
    perror("Error: write in wfich");
    return -1;
  }
  return 0;
}

