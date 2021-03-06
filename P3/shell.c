#include "exinf.h"
#include "dynlist.h"
#include "p0commands.h"
#include "p1commands.h"
#include "p2commands.h"
#include "p3commands.h"

/*
  Sistemas Operativos
  Práctica 1
  Carlos Torres (carlos.torres)
  Daniel Sergio Vega (d.s.vega)
  Grupo 4.3
*/

//Shell:

//Funciones para leer los comandos de la Shell

void printPrompt(){
    printf("# ");
}

int readInput(char comando[], struct extra_info * ex_inf){
    if (fgets(comando,MAXLEN,stdin)==NULL){
        printf("stdin could not be read\n");
        return SHELL_EXIT_FAILURE; //returns an exit signal that tells main() to clean up and exit
    }
    if (comando[0]!='\n'){
        char * com = malloc(256*sizeof(char));
        sprintf(com,"%s\b ",comando);
        InsertElement(&(ex_inf->historial), com);
    }
    return 0;
}

int TrocearCadena(char * cadena, const char * trozos[]){
    int i=1;
    if ((trozos[0]=strtok(cadena," \n\t"))==NULL)
        return 0;
    while ((trozos[i]=strtok(NULL," \n\t"))!=NULL)
        i++;
    return i;
}

int salir(const char *trozos[], int ntrozos, struct extra_info *ex_inf){
  return SHELL_EXIT_SUCCESS; //returns an exit signal that tells main() to clean up and exit
}

//Función para decidir qué comando se va a ejecutar

int processInput(char comando[], struct extra_info *ex_inf){
    const char *trozos[MAX_N_ARG];
    int ntrozos = TrocearCadena(comando,trozos);
    int i = 0;
    int return_value = 0;

    struct {
        char * cmd_name;
        int (* cmd_fun) (const char * trozos[], int ntrozos, struct extra_info *ex_inf);
    } cmds[] = {
        {"autores", autores},
        {"pid", pid},
        {"cdir", cdir},
        {"fecha", fecha},
        {"hora", hora},
        {"hist", hist},
        {"crear", crear},
        {"borrar", borrar},
        {"info", info},
        {"listar", listar},
        {"asignar", asignar},
        {"desasignar", desasignar},
        {"borrarkey", borrarkey},
        {"mem", cmd_mem},
        {"volcar", volcar},
        {"llenar", llenar},
        {"recursiva", recursiva},
        {"rfich", rfich},
        {"wfich", wfich},
        {"priority", priority},
        {"fork",cmdfork},
        {"exec",cmdexec},
        {"pplano",pplano},
        {"splano",splano},
        {"listarprocs", listarprocs},
        {"proc",cmdproc},
        {"borrarprocs",borrarprocs},
        {"fin", salir},
        {"end", salir},
        {"exit", salir},
        {NULL, NULL}
    };

    if (trozos[0]==NULL){ //in case nothing is entered
        return 0;
    }

    for (i = 0; cmds[i].cmd_name != NULL; i++)
    {
        if (!strcmp(trozos[0],cmds[i].cmd_name)){
            return_value = cmds[i].cmd_fun (trozos, ntrozos, ex_inf);
            break;
        } //if
    } //for
    if (cmds[i].cmd_name == NULL){ //cmd not found
      //if command name was not found, will try to run it as an executable
      if (trozos[0][0]=='@'){
        printf("%s is not a shell command. Running as binary executable...\n", trozos[1]);
      }
      else{
        printf("%s is not a shell command. Running as binary executable...\n", trozos[0]);
      }
      return_value = direct_cmd(trozos, ntrozos, ex_inf);
    } //cmd not found
    return return_value;
}

void onexit(struct extra_info * ex_inf) {
  disposeAll(&ex_inf->historial,Free); //cleans up and exits
  disposeMemory(ex_inf);
  disposeAll(&ex_inf->procesos,freePElem);
  free(ex_inf);
};

int main(int argc, const char *argv[]){
    char comando[MAXLEN];
    int ret;

    struct extra_info * ex_inf;
    ex_inf = (struct extra_info *) malloc(sizeof(struct extra_info));

    ex_inf->historial = CreateList();
    ex_inf->memoria = CreateList();
    ex_inf->procesos = CreateList();


    do{
        printPrompt();
        readInput(comando, ex_inf);
        ret = processInput(comando, ex_inf);
    } while((ret != SHELL_EXIT_SUCCESS) && (ret != SHELL_EXIT_FAILURE));

    onexit(ex_inf);

    if (ret == SHELL_EXIT_SUCCESS) return EXIT_SUCCESS;
    if (ret == SHELL_EXIT_FAILURE) return EXIT_FAILURE;

    return 0;
}
