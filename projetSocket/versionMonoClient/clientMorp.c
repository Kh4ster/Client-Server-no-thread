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

void startGame(){
	
	/*read(socket, buffer, LG_MESSAGE * sizeof(char));

	printf("%s", buffer);

	int xy[2];
	scanf("%d %d", &xy[0], &xy[1]);

	write(socket, xy, sizeof(xy));*/
}

void chooseLobby(int socket, char *buffer){

	read(socket, buffer, LG_MESSAGE * sizeof(char));

	int choice;

	while(strcmp(buffer, "wait") != 0 || strcmp(buffer, "play") != 0 )	//Tant qu'on a pas reçu wait ou play on doit choisi un salon
	{
		printf("%s", buffer);	//On affihe l'état des salons de jeu
		scanf("%d", &choice);
		write(socket, &choice, sizeof(int));	//On envoie au serveur notre choix
	}

	printf("ok !");

	exit(-1);

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
