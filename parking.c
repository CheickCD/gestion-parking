#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FICHIER "parking.txt"
#define MAX 100

typedef struct {
    char immat[20];
    time_t entree;
    int aPayer;   // 0 = en cours, 1 = en attente paiement
    int montant;  // montant calculé
} Vehicule;

int main() {
    printf("Content-Type: application/json\n\n");

    char *query = getenv("QUERY_STRING");
    char action[20] = "";
    char immat[20] = "";
    int places = 0;

    if (query) {
        char *p;
        p = strstr(query, "action=");
        if (p) sscanf(p + 7, "%[^&]", action);

        p = strstr(query, "immat=");
        if (p) sscanf(p + 6, "%[^&]", immat);

        p = strstr(query, "places=");
        if (p) sscanf(p + 7, "%d", &places);
    }

    FILE *f;
    int total = 0;
    Vehicule vehicules[MAX];
    int nb = 0;

    // Lecture fichier
    f = fopen(FICHIER, "r");
    if (f) {
        fscanf(f, "%d\n", &total);
        while (fscanf(f, "%s %ld %d %d\n", 
                      vehicules[nb].immat, 
                      &vehicules[nb].entree, 
                      &vehicules[nb].aPayer, 
                      &vehicules[nb].montant) == 4) {
            nb++;
        }
        fclose(f);
    }

    char message[200] = "";

    // INITIALISATION
    if (strcmp(action, "init") == 0) {
        total = places;
        nb = 0;
        sprintf(message, "Parking initialisé avec %d places", total);
    }

    // ENTRÉE
    else if (strcmp(action, "entree") == 0) {
        // Vérifier doublon
        for (int i = 0; i < nb; i++) {
            if (strcmp(vehicules[i].immat, immat) == 0 && vehicules[i].aPayer == 0) {
                printf("{\"message\":\"Vehicule déjà présent\",\"places\":[],\"vehicules\":[]}");
                return 0;
            }
        }

        if (nb < total) {
            strcpy(vehicules[nb].immat, immat);
            vehicules[nb].entree = time(NULL);
            vehicules[nb].aPayer = 0;
            vehicules[nb].montant = 0;
            nb++;
            sprintf(message, "Vehicule entré");
        } else {
            sprintf(message, "Parking plein");
        }
    }

    // SORTIE
    else if (strcmp(action, "sortie") == 0) {
        int trouve = -1;
        for (int i = 0; i < nb; i++) {
            if (strcmp(vehicules[i].immat, immat) == 0 && vehicules[i].aPayer == 0) {
                trouve = i;
                break;
            }
        }

        if (trouve == -1) {
            sprintf(message, "Vehicule non trouvé ou déjà en attente paiement");
        } else {
            time_t maintenant = time(NULL);
            double minutes = difftime(maintenant, vehicules[trouve].entree) / 60.0;
            int tranches = (minutes <= 0) ? 1 : (int)((minutes + 29) / 30);
            int montant = tranches * 100;

            vehicules[trouve].montant = montant;
            vehicules[trouve].aPayer = 1; // Passage en attente paiement

            sprintf(message, "Montant à payer : %d FCFA", montant);
        }
    }

    // PAIEMENT
    else if (strcmp(action, "payer") == 0) {
        int trouve = -1;
        for (int i = 0; i < nb; i++) {
            if (strcmp(vehicules[i].immat, immat) == 0 && vehicules[i].aPayer == 1) {
                trouve = i;
                break;
            }
        }

        if (trouve == -1) {
            sprintf(message, "Vehicule non trouvé ou paiement déjà effectué");
        } else {
            sprintf(message, "Paiement confirmé, montant : %d FCFA", vehicules[trouve].montant);
            // Supprimer le véhicule du tableau
            for (int i = trouve; i < nb - 1; i++) {
                vehicules[i] = vehicules[i + 1];
            }
            nb--;
        }
    }

    else {
        sprintf(message, "Etat actuel");
    }

    // Sauvegarde fichier avec le nouveau format
    f = fopen(FICHIER, "w");
    fprintf(f, "%d\n", total);
    for (int i = 0; i < nb; i++) {
        fprintf(f, "%s %ld %d %d\n", 
                vehicules[i].immat, 
                vehicules[i].entree, 
                vehicules[i].aPayer, 
                vehicules[i].montant);
    }
    fclose(f);

    // JSON
    printf("{\"message\":\"%s\",\"places\":[", message);
    for (int i = 0; i < total; i++) {
        int occupe = 0;
        for (int j = 0; j < nb; j++) {
            if (vehicules[j].aPayer == 0 && j == i) { occupe = 1; break; }
        }
        printf("%d", occupe);
        if (i < total - 1) printf(",");
    }
    printf("],\"vehicules\":[");
    for (int i = 0; i < nb; i++) {
        char heureStr[20];
        struct tm *tm_info = localtime(&vehicules[i].entree);
        strftime(heureStr, 20, "%H:%M:%S", tm_info);
        printf("{\"immat\":\"%s\",\"heure\":\"%s\",\"aPayer\":%d,\"montant\":%d}", 
               vehicules[i].immat, heureStr, vehicules[i].aPayer, vehicules[i].montant);
        if (i < nb - 1) printf(",");
    }
    printf("]}");

    return 0;
}
