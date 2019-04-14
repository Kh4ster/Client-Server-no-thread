#include <stdio.h>
#include <stdlib.h> // pour exit
#include <unistd.h> // pour sleep
#include <string.h> // pour memset

#include <sys/types.h> // pour socket
#include <sys/socket.h>

#include <errno.h>

#include <signal.h>

#include <netinet/in.h> // pour struct sockaddr_in
#include <arpa/inet.h> // pour htons et inet_aton

#include <fcntl.h>  // Pour rendre le accept() et read() non bloquer


#define PORT IPPORT_USERRESERVED // = 5000
#define LG_MESSAGE 256
#define lobbySize 10

/* ---- Logique du morpion  ---- */

// CONVERTIR 3 CHAR EN 1 STRING
char *convertString(char un, char deux, char trois){

    char *str = malloc((3+1) * sizeof(char));

    str[0] = un;
    str[1] = deux;
    str[2] = trois;
    str[3] = '\0';

    return str;
}

// VERIFIER SI LIGNE REGARDER EST FINI
int check(char *ligne, char symbole){
    char *resultatVoulu = convertString(symbole, symbole, symbole); // "XXX" ou "OOO"

    if(strcmp(ligne, resultatVoulu) == 0)
        return 1;
    else
        return 0;
}

// PASSE SUR TOUTES LES LIGNES / COLONNES / DIAGONALES
int checkIfWin(char tab[3][3], char symboleJoueur){

    char *ligne;

    for(int i=0; i < 3; i++){ // ON BOUCLE POUR CHECKER LES LIGNES
        ligne = convertString(tab[i][0], tab[i][1], tab[i][2]);
        if( check(ligne, symboleJoueur) == 1 )
            return 1;
    }
    
	for(int i=0; i < 3; i++){ // ON BOUCLE POUR CHECKER LES COLONNES
        ligne = convertString(tab[0][i], tab[1][i], tab[2][i]);
        if( check(ligne, symboleJoueur) == 1 )
            return 1;
    }
    
	// CHECKER DIAGONALE DE HAUT VERS BAS
    ligne = convertString(tab[0][0], tab[1][1], tab[2][2]);
    if( check(ligne, symboleJoueur) == 1 )
        return 1;
   
    // CHECKER DIAGONALE DE BAS VERS HAUT
    ligne = convertString(tab[2][0], tab[1][1], tab[0][2]);
    if( check(ligne, symboleJoueur) == 1 )
        return 1;

    return 0;
}

// CHECKER SI LES COORDONNEES ENVOYER SONT BIEN A L'INTERIEUR DE LA MATRIX
int coordIsNotOk(int coordonnees[2], char tab[3][3]){
    for(int i = 0; i < 2; i++){
        if(coordonnees[i] < 0 || coordonnees[i] > 2)
            return 1;
    }
    if(tab[coordonnees[0]][coordonnees[1]] != ' ')
    	return 1;
    return 0;
}
// --------------------------------------
// --------------------------------------
// --------------------------------------

typedef struct{

	int nbJoueur;
	char matrix[3][3];
	int indiceJoueur;
	int sockets[2];
	int pipeWriteToMainProc;
	int domainReadFromLobby;

}SalonDeJeu;

/*
* Define provenant du code prix pour les domain socket UNIX
*/
#define LOGD(...) do { printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGE(...) do { printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGW(...) do { printf(__VA_ARGS__); printf("\n"); } while(0)


/**
 * Code provenant d'internet pour utiliser les domain socket UNIX
 *
 * Receives file descriptor using given socket
 *
 * @param socket to be used for fd recepion
 * @return received file descriptor; -1 if failed
 *
 * @note socket should be (PF_UNIX, SOCK_DGRAM)
 */
int recvfd(int socket) {
    int len;
    int fd;
    char buf[1];
    struct iovec iov;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    char cms[CMSG_SPACE(sizeof(int))];

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);

    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = (caddr_t) cms;
    msg.msg_controllen = sizeof cms;

    len = recvmsg(socket, &msg, 0);

    if (len < 0) {
        LOGE("recvmsg failed with %s", strerror(errno));
        return -1;
    }

    if (len == 0) {
        LOGE("recvmsg failed no data");
        return -1;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    memmove(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

/**
 * Code provenant d'internet pour utiliser les domain socket UNIX
 *
 * Sends given file descriptior via given socket
 *
 * @param socket to be used for fd sending
 * @param fd to be sent
 * @return sendmsg result
 *
 * @note socket should be (PF_UNIX, SOCK_DGRAM)
 */
int sendfd(int socket, int fd) {
    char dummy = '$';
    struct msghdr msg;
    struct iovec iov;

    char cmsgbuf[CMSG_SPACE(sizeof(int))];

    iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = CMSG_LEN(sizeof(int));

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    *(int*) CMSG_DATA(cmsg) = fd;

    int ret = sendmsg(socket, &msg, 0);

    if (ret == -1) {
        LOGE("sendmsg failed with %s", strerror(errno));
    }

    return ret;
}

char* getFormatedMatrix(SalonDeJeu *salonDeJeu, char *buffer)
{
	buffer[0] = 0;
	strcat(buffer, "Etat du morpion :\n");
	int curr = strlen(buffer);
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
			buffer[curr++] = salonDeJeu->matrix[i][j];
		buffer[curr++] = '\n';
	}
	buffer[strlen(buffer)] = 0;
	strcat(buffer, "Entrez la case au format : x y\n");
	return buffer;
}

void fillMatrix(char matrix[3][3])
{
	for(int i = 0; i < 3; ++i)
		for(int j = 0; j < 3; ++j)
			matrix[i][j] = ' ';
}


void initFieldSalon(SalonDeJeu *salonsDeJeu)
{
	salonsDeJeu->nbJoueur = 0;
	salonsDeJeu->indiceJoueur = 0;
	salonsDeJeu->sockets[0] = -1;
	salonsDeJeu->sockets[1] = -1;
	fillMatrix(salonsDeJeu->matrix);
}

void handlePipeError(int pipeError)
{
	if(pipeError == -1){perror("pipe"); exit(-1);}
}

void writeToClient(int socketTalk, char *message, char *buffer){
	sprintf(buffer, "%s", message);
	write(socketTalk, buffer, strlen(buffer));
}

void handleGame(SalonDeJeu *salonDeJeu)
{
	char buffer1[LG_MESSAGE];
	char buffer2[LG_MESSAGE];
	memset(buffer1, 0x00, LG_MESSAGE * sizeof(char));
	memset(buffer2, 0x00, LG_MESSAGE * sizeof(char));

	int coordonnees[2];	//Tableau dans lequel on va ranger les coordonnées reçues

	char joueurActuel = 'X';

	writeToClient(salonDeJeu->sockets[1], "wait", buffer1);	//On indique au deuxième client qu'il doit attendre

	while(!checkIfWin(salonDeJeu->matrix, (joueurActuel == 'X') ? 'O' : 'X'))	//On check ce que le joueur d'avant à joué
	{
		do // On lis la réponse du client TANT QU'IL N'A PAS ENVOYER DE BONNE COORDONNEES
		{
			
			writeToClient(salonDeJeu->sockets[salonDeJeu->indiceJoueur], getFormatedMatrix(salonDeJeu, buffer2), buffer2); //On donne l'état de la matrice au ième joueur
			
			memset(buffer2, 0x00, LG_MESSAGE * sizeof(char));
			
			read(salonDeJeu->sockets[salonDeJeu->indiceJoueur], coordonnees, sizeof(coordonnees));	//On lie les coordonnées envoyés par le client
		
		}while(coordIsNotOk(coordonnees, salonDeJeu->matrix)); 

        writeToClient(salonDeJeu->sockets[salonDeJeu->indiceJoueur], "okcoord", buffer1);	//On indique au client qu'il a rentré de bonne coordonnée

        memset(buffer1, 0x00, LG_MESSAGE * sizeof(char));

        salonDeJeu->matrix[coordonnees[1]][coordonnees[0]] = joueurActuel;	//On indique dans la matrice la lettre mis par le joueur

		salonDeJeu->indiceJoueur = (salonDeJeu->indiceJoueur + 1) % 2;	//On incrémente et %2 pour alterner 0 1

		joueurActuel = (salonDeJeu->indiceJoueur == 1) ? 'O' : 'X';						//Si c'est au joueur 1, le lettre est maintenant O sinon X
	}

	memset(buffer1, 0x00, LG_MESSAGE * sizeof(char));
	memset(buffer2, 0x00, LG_MESSAGE * sizeof(char));

	writeToClient(salonDeJeu->sockets[0], "fini", buffer1);

	writeToClient(salonDeJeu->sockets[1], "fini", buffer2);

	printf("Le joueur %c a gagné !\n", (joueurActuel == 'X') ? 'O' : 'X'); //C'est le dernier à avoir joué qui gagne

	/*On signale au procesus main que ce salon est libre*/

	salonDeJeu->nbJoueur = 0;

	write(salonDeJeu->pipeWriteToMainProc, &salonDeJeu->nbJoueur, sizeof(int));

	//On sort de cette fonction et on retourne dans la waiting room

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
	while(1)	//La salle reste tout le temps en vie
	{
		initFieldSalon(salonsDeJeu);	//Avant chaque nouvelle attente on remet tout à 0;
		
		while(salonsDeJeu->nbJoueur < 2)	//Tant qu'il n'y a pas 2 joueur on attent
		{
			/*
			*	Ici on attend que un processus de lobby nous envoie un descripteur de socket pour jouer
				recvf est une fonction pour recevoir un file descriptor
			*	Ici le read est bloquant
			*/

			salonsDeJeu->sockets[salonsDeJeu->nbJoueur] = recvfd(salonsDeJeu->domainReadFromLobby);

			//Une fois qu'on a recu un nouveau joueur on incrémente le nombre de joueur par 1

			salonsDeJeu->nbJoueur += 1;

			//On signal ensuite on processus père qu'on a un nouveau nombre de joueur

			write(salonsDeJeu->pipeWriteToMainProc, &salonsDeJeu->nbJoueur, sizeof(int));

		}

		printf("Les deux joueurs sont là, c'est partie !\n");

		handleGame(salonsDeJeu);	//Une fois qu'on a nos 2 joueurs/sockets la partie peut commencer

	}
}

void handleLobby(SalonDeJeu *salonsDeJeu, int domainWriteSocket[lobbySize][2], int socketTalk)	//Ici les clients choissient leur salon
{
	char buffer[LG_MESSAGE];
	memset(buffer, 0x00, LG_MESSAGE * sizeof(char));
	
	
	int salonChoisi = -1919;	
	
	/*	De base on met ce chiffre
	*	Si le client se déconnecte et que ce chiffre est toujours là on sait que c'est car il s'est déconnecté
	*	On ne met pas -1 car il peut se tromper et mettre -1, dans ce cas on lui redemande un salon
	*/ 

	do
	{
		writeToClient(socketTalk, getSalonInfo(salonsDeJeu), buffer);	//On lui donne l'état actuel des salons

		read(socketTalk, &salonChoisi, sizeof(int));		//On lit quel salon il a choisi

	}while(testRoom(salonsDeJeu, salonChoisi) == -1);

	if(salonChoisi == -1919)	//Si le joueur s'est déconnecté pendant on détruit le processus
		exit(-1);

	//On dit on joueur que sa sélection est bonne et qu'il est bien dans le salon

	writeToClient(socketTalk, "ok", buffer);

	salonsDeJeu[salonChoisi].nbJoueur += 1;	//On incrémente de 1 le nombre de joueur il a choisi

	sendfd(domainWriteSocket[salonChoisi][1], socketTalk);	//On indique au salon de jeu qu'on a un nouveau client pour client

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

void handleDomaineSocketError(int error)
{
	if(error < 0)
	{
		perror("bind domain socket");
		exit(-1);
	}
}

void initSalonsDeJeu(SalonDeJeu *salonsDeJeu, int tubesReadInfo[lobbySize][2], int domainWriteSocket[lobbySize][2])
{
	for(int i = 0; i < lobbySize; ++i)		//On initaliser les salons de jeu et on les lances
	{
		initFieldSalon(&salonsDeJeu[i]);
		salonsDeJeu[i].pipeWriteToMainProc = tubesReadInfo[i][1];			//On lui passe ce bout de la pipe pour qu'il donne ses infos sur ses états
		salonsDeJeu[i].domainReadFromLobby = domainWriteSocket[i][0];			//On lui passe ce bout de la pipe pour qu'il puisse récupérer des joueurs
		int error;
		if( (error = fork()) == 0)
		{
			handleForkError(error);
			/*close(tubesReadInfo[i][0]);			//Le père ferme le côté écriture de son côté
			close(domainWriteSocket[i][0]);		//Et le côté lecture socket de son côté*/

			//On envoie le processus et le salon de jeu dans une fonction où il attendra des joueurs et lancera des parties
			waitingRoom(&salonsDeJeu[i]);
		}

		/*close(tubesReadInfo[i][1]);			//Le père ferme le côté écriture de son côté
		close(domainWriteSocket[i][0]);		//Et le côté lecture socket de son côté*/

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
	*
	* Pour communiquer entre les salons de jeu et les lobbys on n'utilise pas des pipes mais des UNIX domain socket
	* Ainsi on pourra passer les descripteurs de socket en paramètre
	* Via les pipes, passer juste la valeur de la socket ne suffit pas pour communiquer avec
	*/

	int tubesReadInfo[lobbySize][2];	//Ce tableau sera utilisé pour actualiser les infos des salons de jeu en gardant un lien avec les processus de salons

	int domainWriteSocket[lobbySize][2];	//Ce tableau sera utilisé pour faire entrer les clients dans les salons de jeu

	/*
	 *	Pour une raison que j'ignore quand je fais l'initialisation via une fonction
	 *  Les pipes ne fonctionnent pas
	*/

	int error = 0;
	for(int i = 0; i < lobbySize; ++i)
	{
		error = pipe2(tubesReadInfo[i], O_NONBLOCK);	//Cette pipe est non boquante
		handlePipeError(error);
		error = socketpair(PF_UNIX, SOCK_DGRAM, 0, domainWriteSocket[i]);
        handleDomaineSocketError(error);
	}

	// Une fois les tubes initialisés on va initier les salons et donc les 15 processus

	initSalonsDeJeu(salonsDeJeu, tubesReadInfo, domainWriteSocket);

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
			read(tubesReadInfo[i][0], &state, sizeof(int));
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
				handleLobby(salonsDeJeu, domainWriteSocket, socketTalk);
			}
		}

		//Le père continue à lire l'état des tubes et à acceullir des clients si nécessaire
	}

	// Fermeture de la socket d'ecoute
	close(socketListen);
	return EXIT_SUCCESS;
}
