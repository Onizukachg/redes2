#include "libRedes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


/*
 * Autores:
 *
 * Alonso Luna, Miguel
 * Galan Galiano, Cristian
 *
 * */


/* NOTA IMPORTANTE:
 * El comando shell(), as� como el algoritmo de parseo de argumentos en main()
 * ha sido tomado de la practica de Sistemas Distribuidos, pero modificandolo
 * para cumplir los requisitos que se nos piden, ya sea de funcionalidad como
 * los distintos comandos o de restriccion como el uso de librerias especificas
 *
 * El algoritmo de select() ha sido tomado casi en su totalidad del libro Beej
 * */


void shell();

/**
 * Atributos Globales:
 * */

struct inicio {
/**
 * Para todos los casos el valor 1 significa que se ha incluido de inicio. 0 que no.
 */
    unsigned int serv:1;
    char* servername;
    char* port;

    unsigned int nick:1;
    char* nickname;

    unsigned int channel:1;
    char* channelname;
    unsigned int debug:1;
} estado;

struct palabra {
    int cantidad;
    char** nombre;
} palabra;

int serverConnected;
fd_set descriptorLectura;
fd_set copia;
int fdmax;
int quit;

/**
 * Funciones
 * */

void usage(char *program_name) {
        printf("Usage: %s [-s host:port] [-n nick] [-c channel] [-d] \n", program_name);
}

/* Obtiene el host y el port que se le va a pasar por parametro */
void obtenerHostPort(char* host) {
        char* aux;

        estado.serv = 1;
        estado.servername = strtok(host, ":");
        aux = strtok(NULL, ":");
        if(aux != NULL){
                estado.port = aux;
        }else{
                estado.serv = 0;
                estado.servername = NULL;
                printf("Formato equivocado de <server:port> \n");
        }
}

void shell() {
        char line[1024];
		char *pch;
		char auxLine[1024];
		char* aux;
		char** auxArray;
		int i = 1;

		memset(line, 0, 1024);
		pch = fgets(line, 1024, stdin);
		strcpy(auxLine,line);

		if(pch != NULL){
			if ( (strlen(line)>1) && ((line[strlen(line)-1]=='\n') || (line[strlen(line)-1]=='\r')) )
				line[strlen(line)-1]='\0';

			strtok(auxLine, " ");
			palabra.cantidad = 1;
			while(strtok( NULL, " ") != NULL){
				palabra.cantidad += 1;
			}

			auxArray = calloc(palabra.cantidad, sizeof(char*));
			aux = strtok(line, " ");
			auxArray[0] = aux;
			while((aux = strtok( NULL, " ")) != NULL){
							auxArray[i] = aux;
							i++;
			}

			palabra.nombre = auxArray;

			if ( (palabra.nombre != NULL) && (palabra.cantidad > 0) ) {
				if (strcmp(palabra.nombre[0],"/help")==0) {
					if (palabra.cantidad == 1)
							c_help();
					else
							printf("Syntax error. Use: /help\n");
				} else if (strcmp(palabra.nombre[0],"/connect")==0) {
					if (palabra.cantidad == 2){
							obtenerHostPort(palabra.nombre[1]);
						if(estado.serv){
							c_connect(&serverConnected, &descriptorLectura, &fdmax, estado.servername, estado.port);
						}
					}else
							printf("Syntax error. Use: /connect <host:port>\n");
				} else if (strcmp(palabra.nombre[0],"/auth")==0) {
					if (palabra.cantidad == 2)
							c_auth(palabra.nombre[1]);
					else
							printf("Syntax error. Use: /auth <nick>\n");
				} else if (strcmp(palabra.nombre[0],"/list")==0) {
					if (palabra.cantidad == 1)
							c_list();
					else
							printf("Syntax error. Use: /list\n");
				} else if (strcmp(palabra.nombre[0],"/join")==0) {
					if (palabra.cantidad == 2){
							c_join(palabra.nombre[1]);

							estado.channelname = calloc(strlen(palabra.nombre[1]), sizeof(char));
							strcpy(estado.channelname, palabra.nombre[1]);
					}else if(palabra.cantidad == 1){
							if(estado.channel == 1)
								printf("*** You are on channel %s\n",estado.channelname);
							else
								printf("*** You aren't on channel\n");
					}else{
							printf("Syntax error. Use: /join [<channel>]\n");
					}
				} else if (strcmp(palabra.nombre[0],"/leave")==0) {
					if (palabra.cantidad == 1)
							c_leave(estado.channelname);
					else
							printf("Syntax error. Use: /leave\n");
				} else if (strcmp(palabra.nombre[0],"/who")==0) {
					if (palabra.cantidad == 1)
							c_who(estado.channelname);
					else
							printf("Syntax error. Use: /who\n");
				} else if (strcmp(palabra.nombre[0],"/info")==0) {
					if (palabra.cantidad == 2){
							c_info(palabra.nombre[1]);
					}else
							printf("Syntax error. Use: /info <user>\n");
				} else if (strcmp(palabra.nombre[0],"/msg")==0) {
					if (palabra.cantidad >= 2){
							aux = "\0";
							for(i = 1;  i < palabra.cantidad; i++){
								aux = strcon(aux, palabra.nombre[i], " ", NULL, NULL);
							}
							c_msg(estado.channelname, aux);
					}else
							printf("Syntax error. Use: /msg <text>\n");
				} else if (strcmp(palabra.nombre[0],"/disconnect")==0) {
					if (palabra.cantidad == 1)
							c_disconnect();
					else
							printf("Syntax error. Use: /disconnect\n");
				} else if (strcmp(palabra.nombre[0],"/quit")==0) {
					if (palabra.cantidad == 1){
							c_quit();
					} else
						printf("Syntax error. Use: /quit\n");
				} else if (strcmp(palabra.nombre[0],"/nop")==0) {
					if (palabra.cantidad == 2)
							c_nop(estado.debug, palabra.nombre[1]);
					else
							printf("Syntax error. Use: /nop [<text>]\n");
				} else if (strcmp(palabra.nombre[0],"/sleep")==0) {
					if (palabra.cantidad == 2){
							c_sleep(estado.debug, palabra.nombre[1]);
					}else if (palabra.cantidad == 1){
							c_sleep(estado.debug, "5");
					}else
							printf("Syntax error. Use: /sleep [<secs>]\n");
				} else if(palabra.nombre[0][0] != '\\'){
						aux = "\0";
						for(i = 0;  i < palabra.cantidad; i++){
							aux = strcon(aux, palabra.nombre[i], " ", NULL, NULL);
						}
						c_msg(estado.channelname, aux);
				} else {
					fprintf(stderr, "Error: command '%s' not valid.\n", palabra.nombre[0]);
				}
			}
			free(auxArray);
		}

}

int main(int argc, char *argv[]){
		char *program_name = argv[0];
        int opt;
        int i;

        /* Parse command-line arguments */
        while ((opt = getopt(argc, argv, "n:s:dc:")) != -1) {
		   switch (opt) {
				   case 'n':
						   estado.nick = 1;
						   estado.nickname = optarg;
						   break;
				   case 'd':
						   estado.debug = 1;
						   break;
				   case 's':
						   obtenerHostPort(optarg);
						   break;
				   case 'c':
						   estado.channel = 1;
						   estado.channelname = optarg;
						   break;
				   case '?':
						   if (optopt == 's')
							fprintf (stderr, "Option -%c requires an argument.\n", optopt);
						   else if (isprint (optopt))
							fprintf (stderr, "Unknown option `-%c'.\n", optopt);
						   else
							fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				   default:
						   usage(program_name);
						   exit(-1);
		   }
        }

        if(estado.debug){
                printf("MODO DEBUG \n");
        }

        FD_ZERO(&descriptorLectura);

        if(estado.serv){
        	c_connect(&serverConnected, &descriptorLectura, &fdmax, estado.servername, estado.port);
                if(estado.nick){
                	c_auth(estado.nickname);
                }
                if(estado.channel){
                	c_join(estado.channelname);
                }
        }else{
        	fdmax = 0;
        }

        FD_SET(0, &descriptorLectura);

        quit = 0;
        while(quit >= 0) {
                copia = descriptorLectura;

                if (select(fdmax+1, &copia, NULL, NULL, NULL) == -1) {
                        perror("select");
                        exit(4);
                }

                for(i = 0; i <= fdmax; i++) {
                        if (FD_ISSET(i, &copia)) {
                        	if (i == 0) {
								shell();
							} else if (i == serverConnected) {
								quit = recibirMensaje();
							}
                        }
                }
                if(quit == 332)
                	estado.channel = 1;
                else if(quit == 451)
                	free(estado.channelname);
        }

        printf("Fin \n");

        if(estado.serv){
                printf("%i \n", serverConnected);
                close(serverConnected);
                exit(EXIT_SUCCESS);
        }else{
                exit(-1);
        }
}
