#include <stdio.h>  // standardna biblioteka printf, fpritnf...
#include <stdlib.h> // funkcije za dinamicko upravljanje memorijom (malloc, free, exit...)
#include <pthread.h> // bilbioteka za rad sa dretvama (pthread_create, pthread_join)
#include <unistd.h> // sleep
#include <time.h>   // ukljucuje rad s vremenom (time - inicijalizacija generatora slucajnih brojeva)

// Globalne varijable
int *stolovi; // Polje za stanje stolova: -1 (slobodan) ili ID dretve (rezerviran)
int *ULAZ, *BROJ; // Varijable za Lamportov algoritam // ULAZ - zapis je li dretva u procesu odabira
                                                      /* BROJ - biljezi prioritet svake dretve
                                                                (0 - nije u kriticnom odjescku) */
int n_dretvi, n_stolova; // Broj dretvi i stolova

// Funkcija za ispis stanja stolova
void ispis_stanja() {
    printf("Stanje: ");
    for (int i = 0; i < n_stolova; i++) {
        printf("Stol %d: ", i + 1);
        if (stolovi[i] == -1) printf("- ");
        else printf("%d ", stolovi[i]);
    }
    printf("\n");
}

// Provjera ima li slobodnih stolova
int ima_slobodnih_stolova() {
    for (int i = 0; i < n_stolova; i++) {
        if (stolovi[i] == -1) return 1;
    }
    return 0;
}

// LAMPORTOV ALGORITAM: osigurava ulaz SAMO JEDNE dretve u kriticni odsjecak
void udi_u_kriticni_odsjecak(int i) {
    ULAZ[i] = 1;    // dretva "uzima broj za red"
    int max_broj = 0;
    for (int j = 0; j < n_dretvi; j++) {    // trazi najveci broj u polju BROJ
        if (BROJ[j] > max_broj) max_broj = BROJ[j];
    }
    BROJ[i] = max_broj + 1;
    ULAZ[i] = 0;    // dretva gotova sa uzimanjem broja
    for (int j = 0; j < n_dretvi; j++) {
    /* ^^provjerava sve ostale dretve kako bi bili sigurni da je dretva[i] na redu */
        while (ULAZ[j] != 0) { /* nista */ } // ceka dok dretva j ne uzme svoj broj
        while (BROJ[j] != 0 && (BROJ[j] < BROJ[i] || (BROJ[j] == BROJ[i] && j < i))) { /* nista */ }
        // ^^^ sve dok dretva j ima manji prioritet ili je ista ali ima manji ID
    }
}

// LAMPORTOV ALGORITAM: izlaz iz kriticnog odsjecka
void izadi_iz_kriticnog_odsjecka(int i) {
    BROJ[i] = 0; // oznacava da dretva[i] vise nije u redu
}

// Funkcija dretve. Prima pokazivac na argument (ID dretve). Vraca pokazivac
void *dretva(void *arg) {
    int id = *(int *)arg; // ID dretve (pocinje od 1)
    srand(time(NULL) + id); // Inicijalizacija SEED-A / poc. vrijednosti
                            // time(NULL) - sekunde od 1.1.1970.
    while (ima_slobodnih_stolova()) {
        sleep(1); // Cekaj 1 sekundu
        int stol = rand() % n_stolova; // Slucajno odaberi stol
        printf("Dretva %d: odabirem stol %d\n", id, stol + 1);

        // Ulaz u kriticni odsjecak
        udi_u_kriticni_odsjecak(id - 1); // samo ID dretva moze gledati popis stolova

        // Provjera i rezervacija stola
        if (stolovi[stol] == -1) {  // ako stol nije rezerviran
            stolovi[stol] = id;     // upisi id dretve
            printf("Dretva %d: rezerviram stol %d, ", id, stol + 1);
            ispis_stanja();
        } else {
            printf("Dretva %d: neuspjela rezervacija stola %d, ", id, stol + 1);
            ispis_stanja();
        }

        /* Izlaz iz kriticnog odsjecka kako bi sljedeca
           dretva mogla uci u kriticni odsjecak */
        izadi_iz_kriticnog_odsjecka(id - 1);
    }
    return NULL;    // svi stolovi rezervirani
}

int main(int argc, char *argv[]) {
    // Provjera argumenata naredbenog retka
    if (argc != 3) {
        fprintf(stderr, "Upotreba: %s <broj_dretvi> <broj_stolova>\n", argv[0]);
        exit(1);
    }

    n_dretvi = atoi(argv[1]);   // pretvaranje argumenata argv[1 ili 2] u int
    n_stolova = atoi(argv[2]);

    // Alokacija memorije
    stolovi = (int *)malloc(n_stolova * sizeof(int));
    ULAZ = (int *)malloc(n_dretvi * sizeof(int));
    BROJ = (int *)malloc(n_dretvi * sizeof(int));
    pthread_t *t_id = (pthread_t *)malloc(n_dretvi * sizeof(pthread_t)); // cuva SLOZENIJE ID-eve dretvi
    int *arg_id = (int *)malloc(n_dretvi * sizeof(int)); // cuva CITLJIVIJE ID-eve dretvi

    // Inicijalizacija
    for (int i = 0; i < n_stolova; i++) stolovi[i] = -1; // Svi stolovi slobodni
    for (int i = 0; i < n_dretvi; i++) {
        ULAZ[i] = 0;
        BROJ[i] = 0;
        arg_id[i] = i + 1; // ID dretve pocinje od 1
    }

    // Stvaranje dretvi
    for (int i = 0; i < n_dretvi; i++) {
        if (pthread_create(&t_id[i], NULL, dretva, &arg_id[i]) != 0) { // 0 == uspjesno
            fprintf(stderr, "Greska pri stvaranju dretve %d!\n", i + 1);
            exit(1);
        }
        /* t_id[i] - polje gdje se spremaju SLOZENIJI ID-evi dretvi
           NULL - zadane postavke dretve (nema posebnih atributa)
           dretva - pokazivac na funkciju dretva
           &arg_id[i] - pokazivac na jednostavnije ID-eve dretvi */
    }

    // Cekanje zavrsetka dretvi
    // pthread_join - ceka zavrsetak dretve (0 == uspjesno)
    for (int i = 0; i < n_dretvi; i++) {
        if (pthread_join(t_id[i], NULL) != 0) {
            fprintf(stderr, "Greska pri zavrsetku dretve %d!\n", i + 1);
            exit(1);
        }
    }

    // Oslobađanje memorije
    free(stolovi);
    free(ULAZ);
    free(BROJ);
    free(t_id);
    free(arg_id);

    return 0;
}
