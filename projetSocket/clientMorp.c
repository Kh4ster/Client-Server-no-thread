#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>		/* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h>  /* pour htons et inet_aton */
#include <unistd.h>
#include <string.h>

#define LG_MESSAGE 256

/*
On se passe les buffer de fonction en fonction pour réduire le coût mémoire
*/

void startGame(int socket, char *buffer){
    memset(buffer, 0x00, LG_MESSAGE * sizeof(char));
	read(socket, buffer, LG_MESSAGE * sizeof(char));	//read est bloquant donc on attend là de savoir qui commence

	int coordonnees[2];

	while(strcmp(buffer, "fini") != 0 && strcmp(buffer, "okcoordfini") != 0)	//Tant qu'on a pas reçu fini on doit choisi un salon
	{
		if(strcmp(buffer, "wait") != 0)	//Celui qui n'a pas recu wait
		{
		    do{
				if(strcmp(buffer, "okcoordfini") == 0) break;	//Je sais que ça ne devrait pas être là mais ça marche et j'ai la flemme
				printf("%s", buffer);								//On affiche l'état du morpion
				scanf("%d %d", &coordonnees[0], &coordonnees[1]);
                memset(buffer, 0x00, LG_MESSAGE * sizeof(char));
                write(socket, coordonnees, sizeof(coordonnees));	//On envoie au serveur notre choix

                memset(buffer, 0x00, LG_MESSAGE * sizeof(char));
                read(socket, buffer, LG_MESSAGE * sizeof(char));	//read est bloquant donc on attend la validation de nos coordonnées envoyé
		    
			}while(strcmp(buffer, "okcoord") != 0); // Si l'on reçoit "okcoord" c'est que les coordonnées que nous avons envoyé sont valide, on peut continuer la partie
		}

		if(strcmp(buffer, "okcoordfini") == 0) break;	//Je sais que ça ne devrait pas être là mais ça marche et j'ai la flemme

		printf("L'autre joueur joue...\n");
		//Sinon ça veut dire qu'on a reçu wait, l'autre joueur joue

		memset(buffer, 0x00, LG_MESSAGE * sizeof(char));
		read(socket, buffer, LG_MESSAGE * sizeof(char));
	}

	printf("La partie est fini !\n");

}

void chooseLobby(int socket, char *buffer){

	int choice;

	read(socket, buffer, LG_MESSAGE * sizeof(char));

	while(strcmp(buffer, "ok") != 0)	//Tant qu'on a pas reçu le message on doit choisi un salon
	{
		printf("%s", buffer);	//On affiche l'état des salons de jeu
		memset(buffer, 0x00, LG_MESSAGE * sizeof(char));	//On clear le buffer à chaque fois de l'ancien message
		scanf("%d", &choice);
		write(socket, &choice, sizeof(int));	//On envoie au serveur notre choix
		read(socket, buffer, LG_MESSAGE * sizeof(char));
	}

	printf("En attente du deuxième joueur\n");

	memset(buffer, 0x00, LG_MESSAGE * sizeof(char));

	startGame(socket, buffer);

}

int main()
{
	/* Partie d'initialisation peu intéressante
	* Aller directement à la partie traitement
	*/

	int socketTalk;
	struct sockaddr_in pointDeRencontreDistant;
	socklen_t longueurAdresse;
	char buffer[LG_MESSAGE]; /* le message de la couche Application ! */
	// Crée un socket de communication
	socketTalk = socket(PF_INET, SOCK_STREAM, 0); /* 0 indique que l’on utilisera le
	protocole par défaut associé à SOCK_STREAM soit TCP */
	//Teste la valeurrenvoyée parl’appel système socket()
	if (socketTalk < 0)
	{					  /* échec ? */
		perror("socket"); // Affiche le message d’erreur
		exit(-1);		  // On sort en indiquant un code erreur
	}
	// Obtient la longueur en octets de la structure sockaddr_in
	longueurAdresse = sizeof(pointDeRencontreDistant);
	// Initialise à 0 la structure sockaddr_in
	memset(&pointDeRencontreDistant, 0x00, longueurAdresse);
	// Renseigne la structure sockaddr_in avec les informations du serveur distant
	pointDeRencontreDistant.sin_family = PF_INET;
	// On choisit le numéro de port d’écoute du serveur
	pointDeRencontreDistant.sin_port = htons(IPPORT_USERRESERVED); // = 5000
	// On choisit l’adresse IPv4 du serveur
	inet_aton("0.0.0.0", &pointDeRencontreDistant.sin_addr); // à modifier selon ses besoins
	// Débute la connexion vers le processus serveur distant
	if ((connect(socketTalk, (struct sockaddr *)&pointDeRencontreDistant, longueurAdresse)) == -1)
	{
		perror("connect");		  // Affiche le message d’erreur
		close(socketTalk); // On ferme la ressource avant de quitter
		exit(-2);				  // On sort en indiquant un code erreur
	}
	printf("Connexion au serveur réussie avec succès !\n");
	//--- Début de l’étape n°4 :
	// Initialise à 0 les messages
	memset(buffer, 0x00, LG_MESSAGE * sizeof(char));
	// Envoie un message au serveur

	/* /Partie init */

	/* ----- Traitement ----- */

	chooseLobby(socketTalk, buffer);

	close(socketTalk);
	return 0;
}
