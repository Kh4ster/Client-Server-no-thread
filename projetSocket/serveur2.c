﻿#include <stdio.h>
#include <stdlib.h> // pour exit
#include <unistd.h> // pour sleep
#include <string.h> // pour memset

#include <sys/types.h> // pour socket
#include <sys/socket.h>

#include <signal.h>

#include <netinet/in.h> // pour struct sockaddr_in
#include <arpa/inet.h> // pour htons et inet_aton

#include <fcntl.h>  // Pour rendre le accept() et read() non bloquer


#define PORT IPPORT_USERRESERVED // = 5000
#define LG_MESSAGE 256
#define lobbySize 1

typedef struct{

	char matrix[3][3];

}Matrix;

typedef struct{

	int nbJoueur;
	Matrix matrix;
	int indiceJoueur;
	int sockets[2];
	int pipeWriteToMainProc;
	int pipeReadFromLobby;

}SalonDeJeu;

char* getFormatedMatrix(SalonDeJeu *salonDeJeu, char *buffer)
{
	buffer[0] = 0;
	strcat(buffer, "Etat du morpion :\n");
	int curr = strlen(buffer);
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
			buffer[curr++] = salonDeJeu->matrix.matrix[i][j] + '0';
		buffer[curr++] = '\n';
	}
	buffer[strlen(buffer)] = 0;
	strcat(buffer, "Entrez la case au format x y\n");
	return buffer;
}

void fillMatrix(char matrix[3][3])
{
	for(int i = 0; i < 3; ++i)
		for(int j = 0; j < 3; ++j)
			matrix[i][j] = ' ';
}

void handlePipeError(int pipeError)
{
	if(pipeError == -1){perror("pipe"); exit(-1);}
}

void writeToClient(int socketTalk, char *message, char *buffer){
	sprintf(buffer, "%s", message);
	write(socketTalk, buffer, strlen(buffer));
}

int gameNotOver(SalonDeJeu *salonDeJeu)	//To Code
{
	return 1;
}

void handleGame(SalonDeJeu *salonDeJeu)
{
	char buffer1[LG_MESSAGE];	//buffer pour lire et écrire
	char buffer2[LG_MESSAGE];	//buffer pour lire et écrire
	memset(buffer1, 0x00, LG_MESSAGE * sizeof(char));
	memset(buffer2, 0x00, LG_MESSAGE * sizeof(char));

	int coordonnees[2];	//Tableau dans lequel on va ranger les coordonnées reçues

	char joueurActuel = 'X';
	
	writeToClient(salonDeJeu->sockets[1], "wait", buffer1);	//On indique au deuxième client qu'il doit attendre

	writeToClient(salonDeJeu->sockets[0], "play", buffer2);	//On indique au premier client qu'il peut jouer

	printf("Message envoyé au deux\n");

	while(1){

	}

	/*while(gameNotOver(salonDeJeu))
	{
		writeToClient(salonDeJeu->sockets[salonDeJeu->indiceJoueur], getFormatedMatrix(salonDeJeu, buffer), buffer); //On donne l'état de la matrice au ième joueur
		
		read(salonDeJeu->sockets[salonDeJeu->indiceJoueur], coordonnees, sizeof(coordonnees));	//On lie les coordonnées envoyés par le client												//On lit ce qu'il a envoyé

		salonDeJeu->matrix.matrix[coordonnees[1]][coordonnees[0]] = joueurActuel;	//On indique dans la matrice la lettre mis par le joueur

		salonDeJeu->indiceJoueur = (salonDeJeu->indiceJoueur + 1) % 2;	//On incrémente et %2 pour alterner 0 1

		joueurActuel = (salonDeJeu->indiceJoueur == 1) ? 'O' : 'X';						//Si c'est au joueur 1, le lettre est maintenant O sinon X
	}*/

}

char *getSalonInfo(SalonDeJeu *salonsDeJeu)
{
	char *str = malloc(300);
	*str = 0;
	strcat(str, "Voici les salons : n° du salon puis : 0 -> vide, 1 -> 1 joueurs, 2 -> partie en cours\n");
	int index = strlen(str);
	for(int i = 0; i < lobbySize; ++i){
		str[index++] = i + '0'; //Write the number as a character in the string
		str[index++] = ' ';		//A space
		str[index++] = salonsDeJeu[i].nbJoueur + '0'; 	//And the value of the lobby
		str[index++] = '\n';
	}
	str[index] = 0;	//We put the end character to be able to use strcat again
	strcat(str, "Choissisez votre salon en entrant le numéro\n");
	return str;
}

int testRoom(SalonDeJeu *salonsDeJeu, int choix)
{
	if(choix < 0 || choix >= lobbySize || salonsDeJeu[choix].nbJoueur == 2)
		return -1;
	return 1;
}

void waitingRoom(SalonDeJeu *salonsDeJeu)
{

	while(salonsDeJeu->nbJoueur < 2)	//Tant qu'il n'y a pas 2 joueur on attent
	{
		/* 
		*	Ici on attend que un processus de lobby nous envoie une socket pour jouer
		*	Ici le read est bloquant
		*/

		read(salonsDeJeu->pipeReadFromLobby, &salonsDeJeu->sockets[salonsDeJeu->nbJoueur], sizeof(int));

		//Une fois qu'on a recu un nouveau joueur on incrémente le nombre de joueur par 1

		salonsDeJeu->nbJoueur += 1;

		//On signal ensuite on processus père qu'on a un nouveau nombre de joueur

		write(salonsDeJeu->pipeWriteToMainProc, &salonsDeJeu->nbJoueur, sizeof(int));

	}

	printf("Les deux joueurs sont là, c'est partie !\n");

	handleGame(salonsDeJeu);	//Une fois qu'on a nos 2 joueurs/sockets la partie peu commencer 
}

void handleLobby(SalonDeJeu *salonsDeJeu, int tubesWriteSocket[lobbySize][2], int socketTalk)	//Ici les clients choissient leur salon
{
	char buffer[LG_MESSAGE]; 
	memset(buffer, 0x00, LG_MESSAGE * sizeof(char));
	int salonChoisi = 0;	

	do
	{
		writeToClient(socketTalk, getSalonInfo(salonsDeJeu), buffer);	//On lui donne l'état actuel des salons

		read(socketTalk, &salonChoisi, sizeof(salonChoisi));		//On lit quel salon il a choisi

	}while(testRoom(salonsDeJeu, salonChoisi) == -1);

	//On dit on joueur que sa sélection est bonne et qu'il est bien dans le salon

	writeToClient(socketTalk, "ok", buffer);
	
	salonsDeJeu[salonChoisi].nbJoueur += 1;	//On incrémente de 1 le nombre de joueur il a choisi

	write(tubesWriteSocket[salonChoisi][1], &socketTalk, sizeof(int));	//On indique au salon de jeu qu'on a un nouveau client pour client

	/*
	*	Le processus fils a fini son travail de traitement avec le client et a passé la socket au salon de jeu
	* 	Il peut maintenant reposer en paix
	*/

	exit(-1);
}

void handleForkError(int pid)
{
	if(pid == -1)
	{
		perror("fork");
		exit(-1);
	}
}

void initSalonsDeJeu(SalonDeJeu *salonsDeJeu, int tubesReadInfo[lobbySize][2], int tubesWriteSocket[lobbySize][2])
{
	for(int i = 0; i < lobbySize; ++i)		//On initaliser les salons de jeu et on les lances
	{
		salonsDeJeu[i].nbJoueur = 0;
		salonsDeJeu[i].indiceJoueur = 0;
		salonsDeJeu[i].sockets[0] = -1;
		salonsDeJeu[i].sockets[1] = -1;
		salonsDeJeu[i].pipeWriteToMainProc = tubesReadInfo[i][1];			//On lui passe ce bout de la pipe pour qu'il donne ses infos sur ses états
		salonsDeJeu[i].pipeReadFromLobby = tubesWriteSocket[i][0];			//On lui passe ce bout de la pipe pour qu'il puisse récupérer des joueurs
		int error;
		if( (error = fork()) == 0)
		{
			handleForkError(error);
			/*close(tubesReadInfo[i][0]);			//Le père ferme le côté écriture de son côté
			close(tubesWriteSocket[i][0]);		//Et le côté lecture socket de son côté*/
			
			//On envoie le processus et le salon de jeu dans une fonction où il attendra des joueurs et lancera des parties
			waitingRoom(&salonsDeJeu[i]);
		}
		
		/*close(tubesReadInfo[i][1]);			//Le père ferme le côté écriture de son côté
		close(tubesWriteSocket[i][0]);		//Et le côté lecture socket de son côté*/

		//Le père lui continue à initialiser les processus fils

	}
}

int main() {

	/* Partie d'initialisation peu intéressante
	* Aller directement à la partie traitement
	*/
	int socketListen;
	int socketTalk;
	struct sockaddr_in adresseLocale, adresseDistante;
	socklen_t longueurAdresse;
	
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
	
	// On fixe la taille de la file d'attente à 15
	if(listen(socketListen, 15) < 0) {
		perror("listen");
		exit(-3);
	}
	printf("Socket placee en ecoute\n");


	/* /Partie init */

	/* ----- Traitement ----- */

	SalonDeJeu salonsDeJeu[lobbySize];

	/*
	* Ici on peut à la fois accepter et lire des tubes
	* Le but étant de mettre à jour constament l'état des salons de jeu mais aussi de toujours accepter les clients
	* On pourrait le faire facilement avec des threads mais le défi est justement de n'utiliser que des processus
	*
	* Pour se faire on rend read() et accept() non bloquant pour pouvoir les traiter en permanance
	* Enfin, il y a un risque vu que le serveur n'accepte pas les clients en permanance que plusieurs clients se retrouvent à attendre que le serveur les accepent
	* Or la file qui gère les sockets en avant est de taille 128 sur mon ordinateur donc suffisante, pour savoir -> cat /proc/sys/net/ipv4/tcp_max_syn_backlog
	*
	* C'est un autre processus qui doit gérer l'acceuil et la prise en compte de leur choix
	* En effet si le processus principal le fait alors il n'est plus en mesure d'accepter les autres clients
	* -> notre but est de pouvoir acceuillir plusieurs clients en même temps
	*/

	int tubesReadInfo[lobbySize][2];	//Ce tableau sera utilisé pour actualiser les infos des salons de jeu en gardant un lien avec les processus de salons
	int tubesWriteSocket[lobbySize][2];	//Ce tableau sera utilisé pour faire entrer les clients dans les salons de jeu

	/*
	 *	Pour une raison que j'ignore quand je fais l'initialisation via une fonction
	 *  Les pipes ne fonctionnent pas
	*/ 

	int error = 0;
	for(int i = 0; i < lobbySize; ++i)
	{
		error = pipe2(tubesReadInfo[i], O_NONBLOCK);	//Cette pipe là est non boquante
		handlePipeError(error);
		error = pipe(tubesWriteSocket[i]);				//Celle là est bloquante
		handlePipeError(error);
	}
	// Une fois les tubes initialisés on va initier les salons et donc les 15 processus

	initSalonsDeJeu(salonsDeJeu, tubesReadInfo, tubesWriteSocket);

	/* 
	*	Une fois que le père a fini d'initaliser les salons de jeu il se met à attendre les clients et à actualiser dès que nécessaire les infos des salons
	*	Pour se faire on passe la socket en mode non bloquant
	*/

	fcntl(socketListen, F_SETFL, O_NONBLOCK);

	int i = 0;	//On déclare le compteur ici pour éviter de redéclarer un int à chaque fois dans le while
	int state = -1;	//Ici la variable qui acceuillera les retour des read des pipes, déclaré aussi en dehors pour les mêmes raisons

	while(1) {

		for(i = 0 ; i < lobbySize; ++i)
		{
			read(tubesReadInfo[i][0], &state, sizeof(state));
			if(state != -1)											//La variable state est toujours maintenu à -1, sa valeur change que si read a pu lire une donnée
			{
				salonsDeJeu[i].nbJoueur = state;	//Si read a pu lire c'est que le ième salon lui a signalé un changement dans le nombre de joueur
				printf("On actualise le nombre de joueur \n");
			}
			state = -1;
		}
		
		socketTalk = accept(socketListen, (struct sockaddr *)&adresseDistante, &longueurAdresse);	//accept renvoie -1 systématiquement quand le buffer de connexion est vide

		if(socketTalk != -1)	//Si un client a été acceuilli
		{
			printf("Un client a été acceuilli !\n");
			if(fork() == 0)	//Un nouveau fils va gérer le client
			{
				handleLobby(salonsDeJeu, tubesWriteSocket, socketTalk);
			}
		}

		//Le père continue à lire l'état des tubes et à acceullir des clients si nécessaire
	}
	
	// Fermeture de la socket d'ecoute
	close(socketListen);
	return EXIT_SUCCESS;
}
