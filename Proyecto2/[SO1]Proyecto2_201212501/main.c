/*------------------------librerias---------------------------*/
#include <stdlib.h>         //instrucciones system
#include <stdio.h>          //getchar
#include <ncurses.h>        //para q se vea mas chulo :*)
#include <sys/sem.h>        //semaforos
#include <pthread.h>        //lecturas del hilo
#include <sys/shm.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>

/*------------------------Constantes---------------------------*/
#define DELAY 30000

/*------------------------mutex---------------------------*/
#ifdef MUTEX
pthread_mutex_t mutexBuffer;        //sincroniza buffer
#endif

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
#else
union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};
#endif

/*------------------------metodos---------------------------*/
//pantallas
void bienvenida();
void bando();
void espera(char jugador);
void fin(char jugador);

void mov_izq();
void mov_der();
void juego();
void *funcionThread (void *parametro);
void *colocar(void *parametro);
void *obtener(void *parametro);
void *tiempo(void *parametro);

/*------------------------variables---------------------------*/
//variables para el semaforo
struct sembuf Operacion;
struct sembuf Operacion2;
key_t Clave;
int Id_Semaforo;

//memoria compartida
FILE *archivocomp;
key_t clavecomp;
char *memComp = NULL;

//hilos
pthread_t idHilo, rhilo, whilo, chilo;

bool jug1,jug2, ledi = false;
int turno, aliens=0, jugador = 0, mem = 0, error_hilo=0;
int x=0,y=0,max_x=0,max_y=0;        //posiciones
int vida=5, ptos=0;                 //jugador 1
int rival=0,vidarival=5, vidarival2 = 5,ptorival=0; //jugador 2
int minutos = 0, segundos = 0;      //cronometro
int naves[19];

int main(int argc, char *argv[])
{
    initscr();              //inicia el modo ncurses

    bienvenida();

	return 0;
}

/* Pantalla de Bienvenida */
void bienvenida() {
    char tecla;

    nodelay(stdscr, 0);
    clear();                        //limpia pantalla

    /*dibuja mensajes*/
    move((LINES/2)-7,(COLS/2)-5);
    addstr("SPACE INVADERS");

    move((LINES/2)-2,(COLS/2)-4);
    addstr("Bienvenido!!");

    move((LINES/2),(COLS/2)-13);
    addstr("Presiona enter para continuar...");

    move((LINES/2)+5,(COLS/2)-9);
    addstr("NATTHALIEE - 201212501");

    move(0,COLS-1);
    refresh();                      //refresca la pantalla
    tecla = getch();                //obtiene el enter

    if(tecla =='\n')
        bando();

}

/* Pantalla de Seleccion de bando */
void bando() {

    nodelay(stdscr, 0);
    clear();                    //limpia pantalla

    /*dibuja mensajes*/
    move((LINES/2)-7,(COLS/2)-10);
    addstr("Selecciona tu bando");

    move((LINES/2)-2,(COLS/2)-17);
    addstr("JUGADOR 1 - DEFENSOR : presiona 1");

    move((LINES/2),(COLS/2)-17);
    addstr("JUGADOR 2 - INVASOR : presiona 2");

    move((LINES/2)+5,(COLS/2)-24);
    addstr("Debes esperar mientras se conecta tu oponente...");

    move(0,COLS-1);
    refresh();                  //refresca la pantalla
    jugador = getch();          //obtiene el enter

    if(jugador == '1' || jugador == '2')
        espera(jugador);
}

/* Pantalla de espera de oponente */
void espera(char jugador) {

    //inicia memoria compartida
    archivocomp = fopen("/tmp/acompartido","w+");
    clavecomp = ftok ("/tmp/acompartido",33);     //obtenemos la clave
    mem = shmget(clavecomp,sizeof(int *)*100,0777 | IPC_CREAT); //solicitud SO cree memoria de 400 bytes
    memComp = (char *) shmat(mem,(char *)0,0); //entramos a la memoria con punteros

    curs_set(FALSE);
    getmaxyx(stdscr, max_y, max_x);

    nodelay(stdscr, 0);
    clear();

    vida = 5;
    vidarival = 5;
    ptos = 0;
    ptorival = 0;
    turno = 1;
    jug1 = false;
    jug2 = false;

    //memoria y naves inicializacion
    int iMem;
    for(iMem = 1; iMem < 23; iMem++){
        if (iMem > 7){
            memComp[iMem] = 1;
        }
        if(iMem < 20){
            naves[iMem] = 1;
        }
    }

    //inicializa semaforo
    Clave = ftok ("/bin/ls", 33);
    Id_Semaforo = semget (Clave, 10, 0600 | IPC_CREAT);

    //chequea el seleccionado
    if (jugador == '1'){
        move((LINES/2)-7,(COLS/2)-14);
        addstr("Seleccionaste:   1. DEFENSOR");

        move((LINES/2),(COLS/2)-19);
        addstr("Espera mientras se conecta invasor...2");

        move(0,COLS-1);
        refresh();                  //refresca la pantalla

        semctl (Id_Semaforo, 0, SETVAL, 0);
        //espera semaforo
        Operacion.sem_num = 0;
        Operacion.sem_op = -1;
        Operacion.sem_flg = 0;

        //sale de la region critica
        semop (Id_Semaforo, &Operacion, 1);

        //juego del jugador 1
        jugador = '1';
        memComp[0]=1;
        memComp[1]=x;
        memComp[2]=vida;
        memComp[3]=ptos;
        error_hilo= pthread_create (&rhilo, NULL, obtener, NULL);
        error_hilo=pthread_create(&whilo,NULL,colocar,NULL);
        error_hilo= pthread_create (&chilo, NULL, tiempo, NULL);


    }else if (jugador == '2'){

        move((LINES/2)-7,(COLS/2)-14);
        addstr("Seleccionaste:   2. INVASOR");

        move((LINES/2),(COLS/2)-19);
        addstr("Espera mientras se conecta defensor...1");

        move(0,COLS-1);

        refresh();                  //refresca la pantalla

        Operacion2.sem_num = 0;
        Operacion2.sem_op = 1;
        Operacion2.sem_flg = 0;

        int i;
        for (i = 0; i<1; i++)
        {
            //levanto semaforo
            semop (Id_Semaforo, &Operacion2, 1);
            //sleep (1);
        }

        //juego del jugador 2
        jugador = '2';
        memComp[4]=1;
        memComp[5]=x;
        memComp[6]=vida;
        memComp[7]=ptos;
        error_hilo= pthread_create (&rhilo, NULL, obtener, NULL);
        error_hilo=pthread_create(&whilo,NULL,colocar,NULL);
        error_hilo= pthread_create (&chilo, NULL, tiempo, NULL);
    }

    #ifdef MUTEX
        pthread_mutex_init (&mutexBuffer, NULL);        //inicio
    #endif
        usleep(DELAY);
        juego();
        shmdt ((char *)memComp);
        shmctl (mem, IPC_RMID,(struct shmid_ds *)NULL); //finaliza el recurso de memoria
        unlink ("\tmp\acompartido");                    //elimina el documento

}

/* Pantalla de fin del juego */
void fin(char jugador) {

    char tecla;
    nodelay(stdscr, 0);
    clear();                    //limpia pantalla

    char ptos1[15];
    char ptos2[15];
    char tiempov[15];
    char *puntos="Puntos: ";
    char *tiempo="Tiempo: ";

    printw("-------------FIN DEL JUEGO---------\n\n");
    sprintf(tiempov,"\n%s%d\n",tiempo,minutos);

    if(jugador=='1')
    {
        sprintf(ptos1,"%s%d\n",puntos,ptos);
        sprintf(ptos2,"%s%d\n",puntos,ptorival);
    }else
    {
        sprintf(ptos2,"%s%d\n",puntos,ptos);
        sprintf(ptos1,"%s%d\n",puntos,ptorival);
    }

    //if(vidarival == 0 || ptos >= 0)
    if(jugador==1)
    {
        if(ptorival>ptos)
        {
            printw("Gana INVASOR - Jugador2  \n");
            printw(ptos2);
            printw("Pierde DEFENSOR - Jugador 1  \n");
            printw(ptos1);
        }else
        {
            printw("Gana DEFENSOR - Jugador 1  \n");
            printw(ptos1);
            printw("Pierde INVASOR - Jugador2  \n");
            printw(ptos2);
        }
    }else
    {
        if(ptorival>ptos)
        {
            printw("Gana DEFENSOR - Jugador 1 \n");
            printw(ptos1);
            printw("Pierde INVASOR - Jugador2  \n");
            printw(ptos2);
        }
        else
        {
            printw("Gana INVASOR - Jugador2 \n");
            printw(ptos2);
            printw("Pierde DEFENSOR - Jugador 1  \n");
            printw(ptos1);
        }
    }

    printw(tiempov);
    //refresh();                  //refresca la pantalla
    tecla = getch();            //obtiene el enter

    if(tecla == '\n')           //al presionar enter regresa a la pantalla de bienvenida
        bienvenida();

}


void juego()
{

#ifdef MUTEX
		pthread_mutex_lock (&mutexBuffer);      //espera y bloquea
#endif
    clear();            //limpia pantalla

    int rivaly = 3;
    int ch;
    y = max_y - 4;

	initscr();			//inicia el ncurses
	raw();				//deshabilita la linea del buffer
	keypad(stdscr, TRUE);//si obtenemos F1,F2
	noecho();			//no echo() mientras hacemos getch

    int movs=0;
    getmaxyx(stdscr, max_y, max_x);

    clear();

    for(movs=0;movs<max_y;movs++)
	{
	    mvprintw(movs,max_x-16,"|");    //linea separacion
	}

    char *clock1="Tiempo: ";
    char *info1="Puntos: ";
    char *nombre1="J1 - Defensor: ";
	char *nombre2="J2 - Invasor: ";

    char clock2[6];
    char info2[10]; //puntos j1
    char info3[10]; //puntos j2
    char name1[18];
	char name2[18];

    //dibuja cronometro
    sprintf(clock2,"%s%d:%d",clock1,minutos,segundos);
    mvprintw(max_y/2,max_x-14,clock2);

    //dibuja puntos
	sprintf(info2,"%s%d",info1,ptos);
	sprintf(info3,"%s%d",info1,ptorival);

	if(jugador=='1')
	{
	    sprintf(name1,"%s%d",nombre1,vida);
	    mvprintw(max_y-2,85,name1);
	    mvprintw(max_y-4,max_x-10,info2);
	    sprintf(name2,"%s%d",nombre2,vidarival);
	    mvprintw(0,85,name2);
	    mvprintw(0,max_x-10,info3);
	}
	else
	{
	    sprintf(name1,"%s%d",nombre2,vida);
	    mvprintw(max_y-2,85,name1);
	    mvprintw(max_y-4,max_x-10,info2);
	    sprintf(name2,"%s%d",nombre1,vidarival);
        mvprintw(0,85,name2);
	    mvprintw(0,max_x-10,info3);
	}

	mvprintw(rivaly,max_x-15-rival,"<--------->");      //barra arriba

	//dibuja naves
    int espaciado=max_x/5;
    int mitady=max_y/2;
    int movs2 = 0;
    for(movs=0;movs<20;movs++)
    {
        if(movs2 == 5){
            movs2 = 0;
        }
        if(movs<5)
        {
            //invasores comunes -- tercera fila
            if(naves[movs]==1)
                mvprintw(mitady+2,(espaciado*movs2)+1,"\\-.-/");
        }
        else if(movs>14)
        {
            //invasores comunes --primera fila
            if(naves[movs]==1)
                mvprintw(mitady-4,(espaciado*movs2)+1,"\\-.-/");
        }
        else if(movs>9)
        {
            //invasores comunes --primera fila
            if(naves[movs]==1)
                mvprintw(mitady-2,(espaciado*movs2)+1,"\\-.-/");
        }
        else
        {
            //invasores fuertes --fila de en medio
            if(naves[movs]==1){
                if(movs2 == 0){
                    mvprintw(mitady,(espaciado*movs2)+1,"\\-.-/");
                }

                if(movs2 == 1){
                    mvprintw(mitady,(espaciado*movs2)+1,"/(-1-)\\");
                }

                if(movs2 == 2){
                    mvprintw(mitady,(espaciado*movs2)+1,"/(-2-)\\");
                }

                if(movs2 == 3){
                    mvprintw(mitady,(espaciado*movs2)+1,"/(-3-)\\");
                }

                if(movs2 == 4){
                    mvprintw(mitady,(espaciado*movs2)+1,"/(-4-)\\");
                }
            }
        }
        movs2++;
        aliens = aliens+1;
    }
    //barra de abajo
    mvprintw(y, x, "<--------->");

	ch = getch();
	switch(ch)
	{
	    case KEY_UP:
            error_hilo= pthread_create (&idHilo, NULL, funcionThread, NULL);
            break;
	    case 'a':
            fin(jugador);
            break;
	    case KEY_LEFT:  //izquierda
            mov_izq();
            break;
	    case KEY_RIGHT: //derecha
            mov_der();
            break;
	    case 'z':   //para salir
            shmdt ((char *)memComp);
            shmctl (mem, IPC_RMID,(struct shmid_ds *)NULL); //finaliza el recurso de memoria
            unlink ("\tmp\acompartido"); //elimina el documento
            endwin();
            return;
            break;
	}

	refresh();
#ifdef MUTEX
		pthread_mutex_unlock (&mutexBuffer);        //desbloquueo
#endif
	juego();			                            //sale del modo ncurses
}

void mov_izq()
{
        if(x!=1)
        {
            mvprintw(y, x, "<--------->");  //imprime barra inferior en posicion xy
            refresh();                      //refresaca pantalla
            usleep(DELAY);                  //pausa entre cada movimiento
            x--;                            //abanza la bola hacia la izquierda
        }
        return;
}
void mov_der()
{
        if(x!=max_x-15)
        {
            mvprintw(y, x, "<--------->");  //imprime barra inferior en posicion xy
            refresh();                      //refresaca pantalla
            usleep(DELAY);                  //pausa entre cada movimiento
            x++;                            //abanza la bola hacia la derecha
        }
        return;
}

void *funcionThread (void *parametro)
{
    int copiax=x;
    int copiay=y;
    int movs=0;
    bool choco=false;
    getmaxyx(stdscr, max_y, max_x);
    while(copiay>0){
#ifdef MUTEX
		pthread_mutex_lock (&mutexBuffer);      //espera y bloquea
#endif
        clear();

        for(movs=0;movs<max_y;movs++)
        {
            mvprintw(movs,max_x-16,"|");
        }
        mvprintw(y, x, "<--------->"); // Dibuja abajo cuando esta disparando
        mvprintw(copiay, copiax, "*"); // Dibuja balas
        refresh();
        usleep(DELAY);       // Shorter delay between movements
        copiay--;                 // Advance the ball to the right
        int espaciado=max_x/5;
        int mitady=max_y/2;
        if(copiay==mitady+2)
        {
            for(movs=0;movs<5;movs++)
            {
                if((copiax==(espaciado*movs)+1) &&(naves[movs]==1))
                {
                    choco=true;
                    naves[movs]=0;
                    ptos=ptos+10;
                }
            }
        }
        else if(copiay==mitady)
        {
            for(movs=0;movs<5;movs++)
            {
                if((copiax==(espaciado*movs)+1) &&(naves[movs+5]==1))
                {
                    choco=true;
                    naves[movs+5]=0;
                    ptos=ptos+15;
                }
            }
        }
        else if(copiay==mitady-2)
        {
            for(movs=0;movs<5;movs++)
            {
                if((copiax==(espaciado*movs)+1) &&(naves[movs+10]==1))
                {
                    choco=true;
                    naves[movs+10]=0;
                    ptos=ptos+10;
                }
            }
        }
        else if(copiay==3)
        {
            if(copiax==max_x-15-rival)
            {
                vidarival2=vidarival2-1;
                if(vidarival2==0)
                {
                    vidarival2=1;
                }
                choco=true;
                ptos=ptos+10;
                ledi=true;
            }
        }
        if(choco==true)
        {
            refresh();
            mvprintw(copiay, copiax, "*"); // Print our "hit" at the current xy position
            usleep(DELAY);       // Shorter delay between movements
            break;
        }
#ifdef MUTEX
    pthread_mutex_unlock (&mutexBuffer);        //desbloqueo
#endif
    }
}

void *colocar(void *parametro)
{
    while( true )
    {
    jug1 = true;
    while( jug2 )
    {
        if( turno == 2 )
        {
        jug1 = false;
        while( turno == 2 ){}
        jug1 = true;
        }
    }
    int movs;
    if(jugador=='1')
    {
        if(ledi==true)
        {
            memComp[6]=vidarival2;
            ledi=false;
        }
        memComp[2]=vida;
        memComp[1]=x;
        memComp[3]=ptos;
        for(movs=0;movs<15;movs++)
        {
            memComp[movs+8]=naves[movs];
        }
    }
    else
    {
        if(ledi==true)
        {
            memComp[2]=vidarival2;
            ledi=false;
        }
        memComp[5]=x;
        memComp[6]=vida;
        memComp[7]=ptos;
        for(movs=0;movs<15;movs++)
        {
            memComp[movs+8]=naves[14-movs];
        }
    }
    turno = 2;
    jug1 = false;
    }
}
void *obtener(void *parametro)
{
    while( true )
    {
    jug2 = true;
    while( jug1 )
    {
        if( turno == 1 )
        {
        jug2 = false;
        while( turno == 1 ){}
        jug2 = true;
        }
    }
    int movs=0;
    if(jugador=='1')
    {
        vidarival=memComp[6];
        ptorival=memComp[7];
        rival=memComp[5];
        for(movs=0;movs<15;movs++)
        {
            naves[movs]=memComp[movs+8];
        }
    }
    else
    {
        vidarival=memComp[2];
        ptorival=memComp[3];
        rival=memComp[1];
        for(movs=0;movs<15;movs++)
        {
            naves[movs]=memComp[22-movs];
        }
    }
    turno = 1;
    jug2 = false;
    }
}

void *tiempo(void *parametro)
{
    while(1)
    {
#ifdef MUTEX
		pthread_mutex_lock (&mutexBuffer);          //bloqueo
#endif

        segundos++;
        if(segundos == 60){
            segundos = 0;
            minutos++;
        }
        char *clock1="Tiempo: ";
        char clock2[6];

        if((vidarival == 0) || (vida == 0) || (ptos >= 100))
        {
            fin(jugador);
        }

        sprintf(clock2,"%s%d:%d",clock1,minutos,segundos);
        mvprintw(max_y/2,max_x-14,clock2);
        refresh();

    /*int espaciado=max_x/5;
    int mitady=max_y/2;
    int movs=0;
    int movs2 = 0;
    for(movs=0;movs<20;movs++)
    {
        if(movs2 == 5){
            movs2 = 0;
        }
        if(movs<5)
        {
            //invasores comunes -- tercera fila
            if(naves[movs]==1)
                mvprintw(mitady+2,(espaciado*movs2)+3,"\\-.-/");
        }
        else if(movs>14)
        {
            //invasores comunes --primera fila
            if(naves[movs]==1)
                mvprintw(mitady-4,(espaciado*movs2)+3,"\\-.-/");
        }
        else if(movs>9)
        {
            //invasores comunes --primera fila
            if(naves[movs]==1)
                mvprintw(mitady-2,(espaciado*movs2)+3,"\\-.-/");
        }
        else
        {
            //invasores fuertes --fila de en medio
            if(naves[movs]==1){
                if(movs2 == 0){
                    mvprintw(mitady,(espaciado*movs2)+3,"\\-.-/");
                }

                if(movs2 == 1){
                    mvprintw(mitady,(espaciado*movs2)+3,"/(-1-)\\");
                }

                if(movs2 == 2){
                    mvprintw(mitady,(espaciado*movs2)+3,"/(-2-)\\");
                }

                if(movs2 == 3){
                    mvprintw(mitady,(espaciado*movs2)+3,"/(-3-)\\");
                }

                if(movs2 == 4){
                    mvprintw(mitady,(espaciado*movs2)+3,"/(-4-)\\");
                }
            }
        }
        movs2++;
    }

    refresh();*/


#ifdef MUTEX
		pthread_mutex_unlock (&mutexBuffer);        //desbloqueo
#endif
        sleep(1);
    }
}




