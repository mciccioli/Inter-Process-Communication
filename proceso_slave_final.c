#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1

#define BUFFER_SIZE 300
#define AUX_SIZE 200
#define COMMAND_SIZE 200
#define PID_SIZE 100

#define EXIT_CHARACTER 'e'

int main(int argc, char *argv[])
{


	int exit = 1; //lo uso como flag para saber cuando tengo que salir
	char buffer[BUFFER_SIZE];
	char pid[PID_SIZE];

	while (exit)
	{
		//uso getline basandome en la práctica 0 ej5
		char *line = NULL;
		size_t linecap = 0;
		ssize_t linelen;
		linelen = getline(&line, &linecap, stdin);
		if (linelen <= 0)
		{
			perror("Error al tratar de leer el nombre del archivo CNF\n");
			free(line);
			return 1;
		}

		if (line[0] != EXIT_CHARACTER)
		{

			line[--linelen] = '\0'; //reemplazo el delimiter character de getline por '\0' para poder usar con éxito stpcpy
			stpcpy(buffer, line);
			sprintf(pid, "\nSlave ID: %d\n", (int)getpid());
			strcat(buffer, pid);
			char command[COMMAND_SIZE];
			char aux[AUX_SIZE];
			
			sprintf(command, "minisat %s | grep -e 'Number of v' -e 'Number of cl' -e 'CPU time' -e 'SATISFIABLE' | sed 's/|//g'| sed 's/: */: /g' | sed 's/^ *//g'", line);
			FILE *fp;
			fp = popen(command, "r");
			while (fgets(aux, AUX_SIZE, fp))
			{ //fgets lee hasta una nueva linea. Así, voy almacenando linea por linea en el buffer
				strcat(buffer, aux);
			}
			pclose(fp);
			strcat(buffer, "\n");
			write(STDOUT, buffer, strlen(buffer)); //mando el buffer que contiene los resultado a la salida estandar(el pipe que va desde el proceso esclavo al proceso aplicación)
		}
		else
		{ //en caso de que se haya insertado el caracter de salida
			exit = 0;
		}
		free(line);
	}
	return 0;
}
