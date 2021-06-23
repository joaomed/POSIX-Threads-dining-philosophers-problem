#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <ncurses.h>

//NCURSES
int height, width;
WINDOW *win;

//número do vizinho à esquerda de  i
#define ANTERIOR (filosofo+(num-1)) % num
//número do vizinho à direita de i  
#define SUCESSOR (filosofo+1) % num

//STATUS
#define PENSANDO 0                  //o filósofo está pensando
#define FOME 1                      //o filósofo está tentando pegar os garfos
#define COMENDO 2                   //o filósofo está comendo

//VETORES DE CONTROLE
int *nfilosofo;                     //vetor para controlar o estado de cada um
int *estado;                        //vetor para controlar o estado de cada um

//ENTRADAS
int num,tcomendo,tpensando;

//REGIÃO CRITICA
sem_t *garfos;                      //um semáforo para cada garfo

//exclusao mútua para as regiões críticas... sempre que entrar na região crítica, apenas aquela thread poderá mexer
sem_t mutex;                        
/*Tendo em conta que há necessidade de garantir a consistência dos recursos,
a utilização mais simples do semáforo é em situações na qual se utiliza o 
princípio da exclusão mútua, isto é, que só um processo é executado de cada vez.
 Para isso utiliza-se um semáforo binário, com inicialização em 1. Esse semáforo 
 binário atua como um mutex. O princípio da exclusão mútua ou mutex, é uma técnica 
 usada em programação concorrente para evitar que dois processos ou threads tenham a
 cesso simultaneamente a um recurso partilhado. */

//FUNÇÕES
void *viver(void *filosofo);
void pegar_garfos(int filosofo);
void largar_garfos(int filosofo);
void testar(int);

//---Representa toda atividade realizada pelo filosofo---
void *viver(void *filosofo)
{   
    sleep(tpensando);                //começa a vida pensando         
    while (1)                        //vida eterna
    { 
        int *filo = filosofo;        //identificação do filosofo
                
        //----------TERMINOU DE PENSAR E AGORA VAI TENTAR COMER
        //pega dois garfos ou bloqueia
        pegar_garfos(*filo); 

        //libera os garfos 
        largar_garfos(*filo);     
    }
    pthread_exit(NULL);
}

void pegar_garfos(int filosofo)         //filosofo de 0 a num-1 tenta pegar os garfos
{

    estado[filosofo] = FOME;            //marca que o filósofo está com Fome
    //-----------ESTA COM FOME---------
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win,9,filosofo*4+35," ");
    mvwprintw(win,9,filosofo*4+36," ");
    wattroff(win, COLOR_PAIR(3)); 
    wrefresh(win); 
    wattron(win, COLOR_PAIR(6));
    mvwprintw(win,10,filosofo*4+35,"%d",filosofo+1);
    wattroff(win, COLOR_PAIR(6));
    wrefresh(win); 
    //---------------------------------  

    sem_wait(&mutex);               //entra na região crítica 
       
    testar(filosofo);               //come se vizinhos não estão comendo

    sem_post(&mutex);               //sai da região crítica
  
    sem_wait(&garfos[filosofo]);    //decrementa, ou se não puder comer, espere para ser avisado
 

    //----------COMER----------
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win,9,filosofo*4+35," ");
    mvwprintw(win,9,filosofo*4+36," ");
    wattroff(win, COLOR_PAIR(2));
    wrefresh(win); 
    wattron(win, COLOR_PAIR(6));
    mvwprintw(win,10,filosofo*4+35,"%d",filosofo+1);
    wattroff(win, COLOR_PAIR(6));
    wrefresh(win); 
    sleep(tcomendo);
    //---------------------------  
    estado[filosofo] = PENSANDO;    //marca que o filósofo está pensando    
    //----------PENSAR----------
    wattron(win, COLOR_PAIR(1));
    mvwprintw(win,9,filosofo*4+35," ");
    mvwprintw(win,9,filosofo*4+36," ");
    wattroff(win, COLOR_PAIR(1));
    wrefresh(win); 
    wattron(win, COLOR_PAIR(6));
    mvwprintw(win,10,filosofo*4+35,"%d",filosofo+1);
    wattroff(win, COLOR_PAIR(6));
    wrefresh(win); 
     
}

void largar_garfos(int filosofo)    //filosofo de 0 a num-1 larga os garfos 
{
    sem_wait(&mutex);               //entra na região crítica
    testar(ANTERIOR);               //vê se o vizinho da esquerda pode comer agora
    testar(SUCESSOR);               //vê se o vizinho da direita pode comer agora
    sem_post(&mutex);               //sai da região crítica
    
    //----------PENSAR----------
    wattron(win, COLOR_PAIR(1));
    mvwprintw(win,9,filosofo*4+35," ");
    mvwprintw(win,9,filosofo*4+36," ");
    wattroff(win, COLOR_PAIR(1));
    wrefresh(win);
    wattron(win, COLOR_PAIR(6));
    mvwprintw(win,10,filosofo*4+35,"%d",filosofo+1);
    wattroff(win, COLOR_PAIR(6));
    wrefresh(win);      
    
    sleep(tpensando);
    //---------------------------  
}

void testar(int filosofo)
{
    if(estado[filosofo] == FOME && estado[ANTERIOR]!=COMENDO && estado[SUCESSOR]!=COMENDO){
        estado[filosofo] = COMENDO;  //marca que o filósofo está comendo 
        sem_post(&garfos[filosofo]); //libera os garfos para quem está esperando 
    }

}

int main(int argc, char *argv[])
{
    initscr(); // Start curses mode

    //Ncurses--------------------------------
    height = 1000;
    width = 1000;

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_GREEN);
    init_pair(2, COLOR_BLACK, COLOR_RED);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);
    init_pair(4, COLOR_BLACK, COLOR_CYAN);
    init_pair(5, COLOR_BLACK, COLOR_WHITE);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);

    win = newwin(height, width, 0, 0);
    refresh();
    //----------------------------------------
    
    //Alocar memória e Entradas terminal-------
    num = atoi(argv[1]);    
    tpensando = atoi(argv[2]);
    tcomendo = atoi(argv[3]);

    garfos =(sem_t*) malloc(num *sizeof(sem_t));
    nfilosofo = malloc(num * sizeof *nfilosofo);
    estado = malloc(num * sizeof *estado);
    //------------------------------------------
    int i, arg[num];
    pthread_t filosofos[num];           //uma thread para cada filósofo

    //Legenda___________________________________________________________________________________________________________________
    for(i=0; i<5;i++){
        wattron(win, COLOR_PAIR(4));
        mvwprintw(win,2,10,"JANTAR DOS FILOSOFOS ");
        wattroff(win, COLOR_PAIR(4)); 
        wrefresh(win);
        usleep(150000);
        wattron(win, COLOR_PAIR(5));
        mvwprintw(win,2,10,"JANTAR DOS FILOSOFOS ");
        wattroff(win, COLOR_PAIR(5)); 
        wrefresh(win);
        usleep(150000);
    }

    wattron(win, COLOR_PAIR(1));
    mvwprintw(win,5,4," ");
    mvwprintw(win,5,5," ");
    mvwprintw(win,5,7,"Pensando ");
    wattroff(win, COLOR_PAIR(1)); 
    wrefresh(win);

    wattron(win, COLOR_PAIR(2));
    mvwprintw(win,7,4," ");
    mvwprintw(win,7,5," ");
    mvwprintw(win,7,7,"Comendo ");
    wattroff(win, COLOR_PAIR(2)); 
    wrefresh(win);

    wattron(win, COLOR_PAIR(3));
    mvwprintw(win,9,4," ");
    mvwprintw(win,9,5," ");
    mvwprintw(win,9,7,"Está com fome ");
    wattroff(win, COLOR_PAIR(3)); 
    wrefresh(win);

    wattron(win, COLOR_PAIR(5));
    mvwprintw(win,7,35,"                                                 Mesa                                                                                            ");
    wattroff(win, COLOR_PAIR(5)); 
    wrefresh(win);
    sleep(3);
    //______________________________________________________________________________________________________________________


    //---Iniciando a área crítica para exclusão mútua
    sem_init(&mutex,0,1);

    //---Iniciando os semáforos para os garfos---  
    for (i = 0; i < num; i++)
    {
        sem_init(&garfos[i],0,0);                    
    }

    //---Criando as threads para os filósofos
    for (i = 0; i < num; i++)
    {   
        usleep(10000);
        wattron(win, COLOR_PAIR(1));
        mvwprintw(win,9,i*4+35," ");
        mvwprintw(win,9,i*4+36," ");
        wattroff(win, COLOR_PAIR(1));
        wrefresh(win); 
        wattron(win, COLOR_PAIR(6));
        mvwprintw(win,10,i*4+35,"%d",i+1);
        wattroff(win, COLOR_PAIR(6));
        wrefresh(win);     
   
        nfilosofo[i]=i;
        estado[i]= PENSANDO;
        pthread_create(&filosofos[i], NULL, viver, &nfilosofo[i]); 

    }

    //---Juntando todas as threads
    for (i = 0; i < num; i++)
    {
        pthread_join(filosofos[i], NULL);
    }

    free(estado);
    free(nfilosofo);
    free(estado);   

    endwin(); //End cueses mode

    return 0;
}
