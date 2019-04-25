Projet client serveur morpion (no thread)

L'affichage ne fonctionne pas pour plus de 10 salons mais ça reste fonctionnel

Il y a des leak memory avec les mallocs

_____________________________


Le projet peut gérer en même temps n clients

Ils choissisent les salons où jouer au morpion

Le défi était de réussir à le faire sans thread, seulement avec des processus, pipe et échange de file descriptor pour les sockets