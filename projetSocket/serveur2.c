#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour sleep */
#include <string.h> /* pour memset */

#include <sys/types.h> /* pour socket */
#include <sys/socket.h>

#include <signal.h>

#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */

#define PORT IPPORT_USERRESERVED // = 5000
#define LG_MESSAGE 256
#define lobbySize 15

typedef struct{

	char matrix[3][3];

}Matrix;

typedef struct{

	int nbJoueur = 0;
	Matrix matrix;
	char indiceJoueur = 'X';
	

}SalonDeJeu;

char* getFormatedMatrix(char matrix[3][3])
{
	char *string = malloc(100);
	string[0] = 0;
	strcat(string, "Etat du morpion :\n");
	int curr = strlen(string);
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
			string[curr++] = matrix[i][j];
		string[curr++] = '\n';
	}
	string[strlen(string)] = 0;
	strcat(string, "Entrez la case au format x y\n");
	return string;
}

void fillMatrix(char matrix[3][3])
{
	for(int i = 0; i < 3; ++i)
		for(int j = 0; j < 3; ++j)
			matrix[i][j] = ' ';
}

void handleGame(SalonDeJeu *salonDeJeu, int socketTalk)
{
	char messageEnvoi[LG_MESSAGE]; 
	int coordonnees[2];
	

	sprintf(messageEnvoi, getFormatedMatrix(matrix));
	write(socketTalk, messageEnvoi, strlen(messageEnvoi));


	read(socketTalk, coordonnees, sizeof(coordonnees));	
	salonDeJeu->matrix[messageRecu[1]][messageRecu[0]] = salonDeJeu->indiceJoueur;
	
	salonDeJeu->indiceJoueur = (salonDeJeu->indiceJoueur == 'X') ? 'O' : 'X';	//Si c'était X O, si c'était O X
}

char *getSalonInfo()
{
	char *str = malloc(200);
	*str = 0;
	strcat(str, "Voici les salons : n° du salon puis : 0 -> vide, 1 -> 1 joueurs, 2 -> partie en cours\n");
	int index = strlen(str);
	for(int i = 0; i < lobbySize; ++i){
		str[index++] = i - '0'; //Write the number as a character in the string
		str[index++] = ' ';		//A space
		str[index++] = salonDeJeu[i].nbJoueur; 	//And the value of the lobby
		str[index++] = '\n';
	}
	str[index] = 0;	//We put the end character to be able to use strcat again
	strcat(str, "Choissisez votre salon en entrant le numéro\n");
	return str;
}

int testRoom(SalonDeJeu *salonsDeJeu, int choix)
{
	if(choix < 0 || choix >= lobbySize || salonDeJeu[choix].nbJoueur == 2)
		return -1;
	return 1;
}

void handleLobby(SalonDeJeu *salonsDeJeu, int socketTalk)
{

	char messageEnvoi[LG_MESSAGE]; 
	int messageRecu;	

	do
	{
		sprintf(messageEnvoi, getSalonInfo());
		write(socketTalk, messageEnvoi, strlen(messageEnvoi));
		read(socketTalk, messageRecu, sizeof(messageRecu));		
	}while(testRoom(salonsDeJeu, messageRecu) == -1);
	
	salonsDeJeu[messageRecu].nbJoueur += 1;
	if(salonsDeJeu[messageRecu].nbJoueur == 1)
		waitingRoom(&salonsDeJeu[messageRecu], socketTalk);
	else if(salonsDeJeu[messageRecu].nbJoueur == 2)
		handleGame(&salonsDeJeu[messageRecu], socketTalk)		
}

void handleSocketError(int socketTalk, int socketListen)
{
	if (socketTalk < 0) 
	{
		perror("accept");
		close(socketTalk);
		close(socketListen);
		exit(-4);
	}
}

void handleForkError(int pid)
{
	if(pid == -1)
	{
		perror("fork");
		exit(-1);
	}
}

int main() {

	/* Partie init */
	int socketListen;
	int socketTalk;
	struct sockaddr_in adresseLocale, adresseDistante;
	socklen_t longueurAdresse;
	
	if (signal(SIGINT, monTraitement) == SIG_ERR) printf("Signal Crtl-C non intercepte\n");
	
	// Creation d'un socket de communication
	socketListen = socket(AF_INET, SOCK_STREAM, 0);
	/* 0 indique que l'on utilisera le protocole par defaut associe a SOCK_STREAM soit TCP */
	
	if(socketListen < 0) {
		perror("socket");
		exit(-1);
	}
	printf("Socket creee avec succes ! (%d)\n", socketListen);
	
	// Preparation de l'adresse d'attachement locale
	longueurAdresse = sizeof(struct sockaddr_in);
	memset(&adresseLocale, 0x00, longueurAdresse);
	adresseLocale.sin_family = AF_INET;
	adresseLocale.sin_addr.s_addr = htonl(INADDR_ANY); // toutes les interfaces locales disponibles
	adresseLocale.sin_port = htons(PORT);
	
	if((bind(socketListen, (struct sockaddr *)&adresseLocale, longueurAdresse)) < 0) {
		perror("bind");
		exit(-2);
	}
	printf("Socket attachee avec succes !\n");
	
	// On fixe la taille de la file d'attente à 5
	if(listen(socketListen, 5) < 0) {
		perror("listen");
		exit(-3);
	}
	printf("Socket placee en ecoute\n");
	
	// boucle d'attente de connexion : en theorie, un serveur attend indefiniment !


	/* /Partie init */

	SalonDeJeu salonsDeJeu[lobbySize];
	int pid;

	while(1) {
		socketTalk = accept(socketListen, (struct sockaddr *)&adresseDistante, &longueurAdresse);
		handleSocketError(socketTalk, socketListen);
		pid = fork();
		handleForkError(pid);
		if(pid == 0)
			handleLobby(salonsDeJeu, socketTalk);

	}
}
	
	// Fermeture de la socket d'ecoute
	close(socketListen);
	return EXIT_SUCCESS;
}
