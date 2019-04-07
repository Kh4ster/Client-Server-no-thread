#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void affichage(char tab[3][3]);
char *convertString(char un, char deux, char trois);
int check(char *ligne, char symbole);
int checkIfWin(char tab[3][3], int NB_LIGNE,char symboleJoueur);



int main()
{
    char tab[3][3] = {
        {'O', 'O', ' '},
        {'X', 'X', ' '},
        {'X', ' ', 'O'}
    };
    int fin = 0;
    int NB_LIGNE = 3;

    char symboleJoueur = 'X';
//    char symboleJoueur = 'O';

    affichage(tab);

    fin = checkIfWin(tab, NB_LIGNE, symboleJoueur);
    if(fin != 0)
        return 1; // WIN :)

    return 0; // RIEN TROUVER :(
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - FUNCTION LOGIQUE  - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -


void affichage(char tab[3][3]){
    for(int i=0; i<3; i++){
        for(int j=0; j<3; j++){
            printf("%c | ", tab[i][j]);
        }
        printf("\n");
    }
}

char *convertString(char un, char deux, char trois){
    int tailleTab = 3;
    char *str = malloc((tailleTab+1) * sizeof(char));

    str[0] = un;
    str[1] = deux;
    str[2] = trois;
    str[3] = '\0';

//    printf("%s", str);
    return str;
}

int check(char *ligne, char symbole){
    printf("\n'%s' : ", ligne);
    char *resultatVoulu = convertString(symbole, symbole, symbole); // "XXX" ou "OOO"

    if(strcmp(ligne, resultatVoulu) == 0){
        printf("VOUS AVEZ GAGNER !");
        return 1;
    }
    else{
        printf("Continuer la partie [ ... ]");
        return 0;
    }
}

int checkIfWin(char tab[3][3], int NB_LIGNE,char symboleJoueur){
    char *ligne;

    for(int i=0; i<NB_LIGNE; i++){ // ON BOUCLE POUR CHECKER LES LIGNES
        ligne = convertString(tab[i][0], tab[i][1], tab[i][2]);
        if( check(ligne, symboleJoueur) == 1 )
            return 1;
    }
    for(int i=0; i<NB_LIGNE; i++){ // ON BOUCLE POUR CHECKER LES COLONNES
        ligne = convertString(tab[0][i], tab[1][i], tab[2][i]);
        if( check(ligne, symboleJoueur) == 1 )
            return 2;
    }
    // CHECKER DIAGONALE DE HAUT VERS BAS
    ligne = ligne = convertString(tab[0][0], tab[1][1], tab[2][2]);
    if( check(ligne, symboleJoueur) == 1 )
        return 3;
    // CHECKER DIAGONALE DE BAS VERS HAUT
    ligne = ligne = convertString(tab[2][0], tab[1][1], tab[0][2]);
    if( check(ligne, symboleJoueur) == 1 )
        return 4;

    return 0;
}
