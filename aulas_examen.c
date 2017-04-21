/* 
 * File:   aulas_examen.c
 * Author: Kiko
 *
 * Created on 18 de abril de 2015, 20:27
 */

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/sem.h> 
#include <errno.h> 
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>

#define N1 33
#define ENTRAR 0
#define SALIR 1
#define TERMINAR -1
#define SECS 300

typedef struct _Mensaje {
    long id; /* Identificador del mensaje*/
    int valor; /* Informacion que se quiere transmitir*/
    int alumno;
} mensaje;

struct msgbuf {
    long mtype; /* type of message */
    int valor;
    int alumno;
};

sig_t set;
int aulaB_aux, aulaA_aux;
int numalumnos = 0; /* Numero total de alumnos*/
int cont_alarm = 0;
int aulaA, aulaB; /*Numero de sitios en el aula*/
int msqid; /*Id de cola de mensajes*/
int *asientosA = NULL, *asientosB = NULL; /*Sitio que opupan los alumnos*/
int asientosA_num = 0, asientosB_num = 0; /*Numero de alumnos en aulas*/
sem_t aA, aB; /*Id semaforo de mutex del aula*/
pid_t proceso;

void manejador_alarm(int s) {
    int i, hilo;
    int aula;
    sem_wait(&aA); /*Aseguramos el mantenimiento de las variables mientras las modificamos*/
    for (i = 0; i < aulaA_aux; i++) {
        if (asientosA[i] != -1) {
            hilo = asientosA[i]; /*Metemos al alumno en el aula libre*/
            asientosA_num--;
            printf("El porfesor del aula A le quita el examen al alumno %d\n", hilo);
        }
    }
    sem_post(&aA);

    i = 0;
    printf("El porfesor del aula A %d se va\n", numalumnos + 1);
    printf("El porfesor del aula A %d se va\n", numalumnos + 2);

    sem_wait(&aB); /*Aseguramos el mantenimiento de las variables mientras las modificamos*/
    for (i = 0; i < aulaB_aux; i++) {
        if (asientosB[i] != -1) {
            hilo = asientosB[i];
            asientosB_num--;
            printf("El porfesor del aula B le quita el examen al alumno %d\n", hilo);
        }
    }
    sem_post(&aB);

    printf("El porfesor del aula B %d se va\n", numalumnos + 3);
    printf("El porfesor del aula B %d se va\n", numalumnos + 4);

    sem_destroy(&aA);
    sem_destroy(&aB);

    free(asientosA);
    free(asientosB);

    msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);

    kill(proceso, SIGKILL);

    return;

}

void *aulaA_f(void *dato) {
    int id = (int) dato;
    int aula;
    mensaje msg;
    sleep(rand() % 5);
    printf("Proceso %d en la cola del aula A\n", id);

    msg.id = numalumnos + 1;
    msg.valor = ENTRAR;
    msg.alumno = id;
    msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0); /*Enviamos peticion de entrar en el aula*/

    msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), id, 0); /*Recibimos respuesta*/
    if (msg.valor == ENTRAR) {/*Si se puede*/
        cont_alarm++;
        printf("Proceso %d haciendo el examen en aula A\n", id);
        sleep(rand() % 10); /*Hacemos el examen*/

        msg.id = numalumnos + 1;
        msg.valor = SALIR;
        msg.alumno = id;
        msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0); /*Solicitamos salir*/

        msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), id, 0); /*Salimos*/
        printf("Proceso %d saliendo del aula A\n", id);
        pthread_exit(NULL);
    } else {/*Si no hay espacio*/
        printf("Proceso %d sin espacio en el aula A cambiando a la B\n", id);
        while (1) {
            msg.id = numalumnos + 2;
            msg.valor = ENTRAR;
            msg.alumno = id;
            msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0); /*Solicitamos entrar*/

            msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), id, 0); /*Respuesta*/
            if (msg.valor == 0) {/*Si entramos hacemos el examen*/
                cont_alarm++;
                printf("Proceso %d haciendo el examen en aula B\n", id);
                sleep(rand() % 10);

                msg.id = numalumnos + 2;
                msg.valor = SALIR;
                msg.alumno = id;
                msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);

                msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), id, 0);
                printf("Proceso %d saliendo del aula B\n", id);
                pthread_exit(NULL);
            }/*Si no repetimos hasta que nos dejan*/
        }
    }
}

void *aulaB_f(void *dato) {
    int id = (int) dato;
    int aula;
    mensaje msg;
    sleep(rand() % 5);
    printf("Proceso %d en la cola del aula B\n", id);

    msg.id = numalumnos + 2;
    msg.valor = ENTRAR;
    msg.alumno = id;
    msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0); /*Enviamos peticion de entrar en el aula*/

    msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), id, 0); /*Recibimos respuesta*/
    if (msg.valor == ENTRAR) {/*Si se puede*/
        cont_alarm++;

        printf("Proceso %d haciendo el examen en aula B\n", id);
        sleep(rand() % 10); /*Hacemos el examen*/

        msg.id = numalumnos + 2;
        msg.valor = SALIR;
        msg.alumno = id;
        msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0); /*Solicitamos salir*/

        msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), id, 0); /*Salimos*/
        printf("Proceso %d saliendo del aula B\n", id);
        pthread_exit(NULL);
    } else {/*Si no hay espacio*/
        printf("Proceso %d sin espacio en el aula B cambiando a la A\n", id);
        while (1) {
            msg.id = numalumnos + 1;
            msg.valor = ENTRAR;
            msg.alumno = id;
            msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0); /*Solicitamos entrar*/

            msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), id, 0); /*Respuesta*/
            if (msg.valor == 0) {/*Si entramos hacemos el examen*/
                cont_alarm++;
                printf("Proceso %d haciendo el examen en aula A\n", id);
                sleep(rand() % 10);

                msg.id = numalumnos + 1;
                msg.valor = SALIR;
                msg.alumno = id;
                msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);

                msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), id, 0);
                printf("Proceso %d saliendo del aula A\n", id);
                pthread_exit(NULL);
            }/*Si no repetimos hasta que nos dejan*/
        }
    }
}

void *profeA(void *dato) {
    int id = (int) dato;
    int flag, i;
    mensaje msg;

    while (1) {
        msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), numalumnos + 1, 0);
        if (msg.valor == TERMINAR) {/*Mensaje de Gestor de terminar*/
            pthread_exit(NULL);
        } else if (msg.valor == ENTRAR) {/*Mensaje de alumno de entrar al aula*/
            sem_wait(&aA);
            if (aulaA * 0.85 > asientosA_num * 1.0) {
                for (i = 0; i < aulaA; i++) {
                    if (asientosA[i] == -1) {
                        asientosA[i] = msg.alumno; /*Metemos al alumno en el aula libre*/
                        asientosA_num++;
                        i = aulaA * 2;
                    }
                }
                msg.id = msg.alumno;
                msg.alumno = 0;
                msg.valor = ENTRAR;
                msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
            } else {
                msg.id = msg.alumno;
                msg.alumno = 0;
                msg.valor = SALIR;
                msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
            }
            sem_post(&aA);
        } else if (msg.valor == SALIR) {

            sem_wait(&aA);
            for (i = 0; i < aulaA; i++) {
                if (asientosA[i] == msg.alumno) {
                    asientosA[i] = -1; /*Sacamos al alumno del aula*/
                    asientosA_num--;
                    i = aulaA * 2;
                }
            }
            sem_post(&aA);

            msg.id = msg.alumno;
            msg.valor = 0;
            msg.alumno = 0;

            msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
        }
    }
}

void *profeB(void *dato) {
    int id = (int) dato;
    int i;
    mensaje msg;

    while (1) {
        msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), numalumnos + 2, 0);
        if (msg.valor == TERMINAR) {/*Mensaje de Gestor de terminar*/
            pthread_exit(NULL);
        } else if (msg.valor == ENTRAR) {/*Mensaje de alumno de entrar al aula*/
            sem_wait(&aB); /*Aseguramos el mantenimiento de las variables mientras las modificamos*/
            if (aulaB * 0.85 > asientosB_num * 1.0) {
                for (i = 0; i < aulaB; i++) {
                    if (asientosB[i] == -1) {
                        asientosB[i] = msg.alumno; /*Metemos al alumno en el aula libre*/
                        asientosB_num++;
                        i = aulaB * 2;
                    }
                }
                msg.id = msg.alumno;
                msg.alumno = 0;
                msg.valor = ENTRAR;
                msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
            } else {
                msg.id = msg.alumno;
                msg.alumno = 0;
                msg.valor = SALIR;
                msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
            }
            sem_post(&aB);
        } else if (msg.valor == SALIR) {

            sem_wait(&aB);
            for (i = 0; i < aulaB; i++) {
                if (asientosB[i] == msg.alumno) {
                    asientosB[i] = -1; /*Sacamos al alumno del aula*/
                    asientosB_num--;
                    i = aulaB * 2;
                }
            }
            sem_post(&aB);

            msg.id = msg.alumno;
            msg.valor = 0;
            msg.alumno = 0;

            msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
        }
    }
}

int main(int argc, char** argv) {
    pthread_t *hilos, profes[4];
    int aula_mayor, num;
    key_t clave;
    mensaje msg;
    proceso = getpid();

    printf("Numero de asientos disponibles en el aula A: ");
    scanf("%d", &aulaA);
    aula_mayor = aulaA;
    printf("Numero de asientos disponibles en el aula B: ");
    scanf("%d", &aulaB);
    if (aulaA < aulaB)
        aula_mayor = aulaB;

    while (numalumnos <= aula_mayor || numalumnos >= aulaA + aulaB) {
        printf("Numero de alumnos que haran el examen: ");
        scanf("%d", &numalumnos);
    }

    if (aulaA % 2 == 0) {
        aulaA = aulaA / 2;
    } else
        aulaA = aulaA / 2 + 1;

    if (aulaB % 2 == 0) {
        aulaB = aulaB / 2;
    } else
        aulaB = aulaB / 2 + 1;

    aulaA_aux = aulaA;
    aulaB_aux = aulaB;

    clave = ftok("/bin/ls", N1);
    if (clave == (key_t) - 1) {
        perror("Error al obtener clave para cola mensajes\n");
        exit(EXIT_FAILURE);
    }

    msqid = msgget(clave, 0600 | IPC_CREAT);
    if (msqid == -1) {
        perror("Error al obtener identificador para cola mensajes");
        return (0);
    }

    sem_init(&aA, 0, 1);
    sem_init(&aB, 0, 1);

    asientosA = (int*) malloc(sizeof (int)*aulaA);
    if (asientosA == NULL) {
        msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);
        return 0;
    }
    for (num = 0; num < aulaA; num++)
        asientosA[num] = -1;

    asientosB = (int*) malloc(sizeof (int)*aulaB);
    if (asientosB == NULL) {
        free(asientosA);
        msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);
        return 0;
    }
    for (num = 0; num < aulaB; num++)
        asientosB[num] = -1;

    hilos = (pthread_t*) malloc(sizeof (pthread_t) * numalumnos);
    if (hilos == NULL) {
        free(asientosA);
        free(asientosB);
        msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);
        return 0;
    }

    numalumnos++; /* Aumento del numero de alumnos para no tener un hilo 0*/

    pthread_create(&profes[0], NULL, profeA, (void*) numalumnos + 1);
    pthread_create(&profes[1], NULL, profeA, (void*) numalumnos + 2);
    pthread_create(&profes[2], NULL, profeB, (void*) numalumnos + 3);
    pthread_create(&profes[3], NULL, profeB, (void*) numalumnos + 4);


    for (num = 1; num < numalumnos; num++) {
        srand(rand());
        if (rand() % 2 == 0)
            pthread_create(&hilos[num], NULL, aulaA_f, (void*) num);
        else
            pthread_create(&hilos[num], NULL, aulaB_f, (void*) num);
    }

    while(cont_alarm != numalumnos - 1);
        if (sigemptyset(&set) != 0)
            return 0;
        alarm(SECS);
        signal(SIGALRM, manejador_alarm);
        /*if (sigsuspend(&set) != 0)
            return 0;*/

    for (num = 1; num < numalumnos; num++)
        pthread_join(hilos[num], NULL);

    /*FINALIZAR PROFES*/
    msg.id = numalumnos + 1;
    msg.valor = -1;
    msg.alumno = 0;
    msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
    msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
    msg.id = numalumnos + 2;
    msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);
    msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);

    pthread_join(profes[0], NULL);
    pthread_join(profes[1], NULL);
    pthread_join(profes[2], NULL);
    pthread_join(profes[3], NULL);

    sem_destroy(&aA);
    sem_destroy(&aB);

    free(asientosA);
    free(asientosB);

    msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);

    printf("TERMINADO\n");

    fflush(stdout);
    sleep(3);

    return (EXIT_SUCCESS);
}


