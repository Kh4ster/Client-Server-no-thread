#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>		/* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h>  /* pour htons et inet_aton */
#include <unistd.h>

#define LG_MESSAGE 256

int main()
{
	int descripteurSocket;
	struct sockaddr_in pointDeRencontreDistant;
	socklen_t longueurAdresse;
	char messageRecu[LG_MESSAGE]; /* le message de la couche Application ! */
	// Crée un socket de communication
	descripteurSocket = socket(PF_INET, SOCK_STREAM, 0); /* 0 indique que l’on utilisera le
	protocole par défaut associé à SOCK_STREAM soit TCP */
	//Teste la valeurrenvoyée parl’appel système socket()
	if (descripteurSocket < 0)
	{					  /* échec ? */
		perror("socket"); // Affiche le message d’erreur
		exit(-1);		  // On sort en indiquant un code erreur
	}
	printf("Socket créée avec succès !(%d)\n", descripteurSocket);
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
	if ((connect(descripteurSocket, (struct sockaddr *)&pointDeRencontreDistant,
				 longueurAdresse)) == -1)
	{
		perror("connect");		  // Affiche le message d’erreur
		close(descripteurSocket); // On ferme la ressource avant de quitter
		exit(-2);				  // On sort en indiquant un code erreur
	}
	printf("Connexion au serveur réussie avec succès !\n");
	//--- Début de l’étape n°4 :
	// Initialise à 0 les messages
	memset(messageRecu, 0x00, LG_MESSAGE * sizeof(char));
	// Envoie un message au serveur

	read(descripteurSocket, messageRecu, LG_MESSAGE * sizeof(char));

	printf("%s", messageRecu);

	int xy[2];
	scanf("%d %d", &xy[0], &xy[1]);

	write(descripteurSocket, xy, sizeof(xy));
	//--- Fin de l’étape n°4 !
	// On ferme la ressource avant de quitter
	close(descripteurSocket);
	return 0;
}
