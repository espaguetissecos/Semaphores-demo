/* 
 * File:   cadena_montaje.c
 * Author: Kiko
 *
 * Created on 12 de abril de 2015, 16:51
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>

#define N1 33
#define N2 35

typedef struct _Mensaje {
    long id; /* Identificador del mensaje*/
    int valor; /* Informacion que se quiere transmitir*/
    char aviso[1024];
} mensaje;

int main(int argc, char** argv) {

    key_t clave, clave2;
    int msqid, msqid2;
    mensaje msg;

    pid_t proceso;
    int i;
    char buffer[1024];
    FILE* F_IN = NULL, *F_OUT = NULL;

    if (argc != 3) {
        printf("Parametros de entrada incorrectos\n");
        return 0;
    }

    clave = ftok("/bin/ls", N1);
    if (clave == (key_t) - 1) {
        perror("Error al obtener clave para cola mensajes\n");
        exit(EXIT_FAILURE);
    }
    
    clave2 = ftok("/bin/ls", N2);
    if (clave2 == (key_t) - 1) {
        perror("Error al obtener clave para cola mensajes\n");
        exit(EXIT_FAILURE);
    }

    msqid = msgget(clave, 0600 | IPC_CREAT);
    if (msqid == -1) {
        perror("Error al obtener identificador para cola mensajes");
        return (0);
    }
    
    msqid2 = msgget(clave2, 0600 | IPC_CREAT);
    if (msqid == -1) {
        perror("Error al obtener identificador para cola mensajes");
        return (0);
    }
    
    msg.id = 0;
    msg.valor = 27;
    msg.aviso[0] = '\0';

    for (i = 0; i < 2; i++) {
        proceso = fork();
        if (i == 0 && proceso == 0) {
            while (1) {
                msgrcv(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 1, 0);
                if (msg.valor == 1) {
                    msg.id = 2; /*Tipo de mensaje*/
                    msg.valor = 1;
                    strcpy(msg.aviso, "STOP");
                    msgsnd(msqid2, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);/* Termino al proceso C*/
                    printf("Proceso B terminado\n");
                    exit(EXIT_SUCCESS);/*Terminar Proceso*/
                }
                
                strcpy(buffer, msg.aviso);
                
                for (i = 0; i < strlen(buffer); i++) {/*Pongo en mayuscula*/
                    if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                        buffer[i] = buffer[i] - 32;
                    }
                }
                
                msg.id = 2; /*Tipo de mensaje*/
                msg.valor = 0;
                strcpy(msg.aviso, buffer);
                msgsnd(msqid2, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);/*Mando al proceso C*/
            }
        }

        if (i == 1 && proceso == 0) {
            F_OUT = fopen(argv[2], "w");

            if (F_OUT == NULL) {
                exit(EXIT_FAILURE);
            }

            while (1) {
                msgrcv(msqid2, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 2, 0);
                if (msg.valor == 1) {
                    printf("Proceso C terminado\n");/*Proceso termina*/
                    fclose(F_OUT);
                    exit(EXIT_SUCCESS);
                }
                strcpy(buffer, msg.aviso);/*Imprimo en arhivo*/
                fprintf(F_OUT, "%s", buffer);
            }
        }
    }

    F_IN = fopen(argv[1], "r");

    if (F_IN == NULL) {
        printf("El archivo no se ha podido abrir\n");
        msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);
        msgctl(msqid2, IPC_RMID, (struct msqid_ds *) NULL);
        return 0;
    }

    while (fgets(buffer, 1024, F_IN) != NULL) {
        msg.id = 1; /*Tipo de mensaje*/
        msg.valor = 0;
        strcpy(msg.aviso, buffer);
        msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);/* Envio mensaje al proceso B*/
    }
    
    msg.id = 1; /*Tipo de mensaje*/
    msg.valor = 1;
    strcpy(msg.aviso, "STOP");
    msgsnd(msqid, (struct msgbuf *) &msg, sizeof (mensaje) - sizeof (long), 0);/* Envio a mensaje B que termine*/
    
    fclose(F_IN);
    
    while(proceso > 0){
        proceso = wait(NULL);
    }
    
    msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);
    msgctl(msqid2, IPC_RMID, (struct msqid_ds *) NULL);
    return (EXIT_SUCCESS);
}

