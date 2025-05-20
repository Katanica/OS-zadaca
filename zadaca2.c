#include <stdio.h>      // potrebni za printf i perror
#include <stdlib.h>     // funkcija exit
#include <unistd.h>     // unistd.h funkcije - fork, sleep getpid
#include <sys/types.h>  // tip pid_t za id procesa
#include <sys/ipc.h>    /* potrebno za meduproc. komunikaciju
                           ukljucujuci zajednicku memoriju */
#include <sys/shm.h>    /* funkcije za rad sa zajednickom
                           memorijom (shmget, shmat, shmdt) */
#include <sys/wait.h>   // funkcija wait
#include <signal.h>     // omogucuje obradu signala (npr. SIGINT)

/*  ZADAĆA 2.
    Dekkerov postupak međusobnog isključivanja
    Ostvariti sustav paralelnih procesa/dretvi.
    Struktura procesa/dretvi dana je sljedećim pseudokodom:
    proces proc(i)
     i [0..n-1]
    za k = 1 do 5 čini
    uđi u kritični odsječak
    za m = 1 do 5 čini
    ispiši (i, k, m)
    izađi iz kritičnog odsječka
    kraj.
    Međusobno isključivanje ostvariti za dva procesa dretve
    međusobnim isključivanjem po Dekkerovom algoritmu */


// Struktura za ZAJEDNICKE varijable
struct shared_data {
    int pravo;          // pravo = 0/1 - oznacava koji proces ima prednost
    int zastavica[2];   // ZASTAVICA[0..1] - niz koji oznacava zelju za ulaskom u program

};

int shmid;               // ID segmenta zajednicke memorije
struct shared_data *shm; // pokazivac na zajednicku memoriju

// Funkcija za obradu signala SIGINT / Ctrl+C
void obradi_sigint(int sig) {   // sig - identifikator signala
    // Odspajanje zajednicke memorije
    shmdt(shm);
    // Unistavanje zajednicke memorije
    shmctl(shmid, IPC_RMID, NULL);  // unistava segment zajednicke memorije
    printf("\nZajednicka memorija ociscena.\n");
    exit(0);
}

// DEKKEROV ALGORITAM za ulazak u kriticni odsjecak
void udi_u_kriticni_odsjecak(int i, int j) {
    shm->zastavica[i] = 1;              // proces i signalizira da zeli uci u k. odsjecak
    while (shm->zastavica[j] == 1) {
        if (shm->pravo == j) {
            shm->zastavica[i] = 0;
            while (shm->pravo == j) {
                    // cekaj
            }
            shm->zastavica[i] = 1;
        }
    }
}

// DEKKEROV ALGORITAM za izlazak iz kriticnog odsjeka
void izadi_iz_kriticnog_odsjecka(int i, int j) {
    shm->pravo = j;
    shm->zastavica[i] = 0;
}

// (PSEUDOKOD); Funkcija koju izvrsava proces dijete
void proces(int i) {
    int j = 1 - i; // drugi proces (0 ako je i=1, 1 ako je i=0)
    for (int k = 1; k <= 5; k++) {
        udi_u_kriticni_odsjecak(i, j);
        // Kriticni odsjecak
        for (int m = 1; m <= 5; m++) {
            printf("Proces %d, k=%d, m=%d\n", i, k, m);
            fflush(stdout);
            sleep(1); // Usporavanje za vidljivost
        }
        izadi_iz_kriticnog_odsjecka(i, j);
    }
    exit(0);    // zavrsava proces dijete
}

int main() {
    // Postavljanje obrade signala SIGINT
    signal(SIGINT, obradi_sigint);

    // Stvaranje zajednicke memorije za struct shared_data
    shmid = shmget(IPC_PRIVATE, sizeof(struct shared_data), IPC_CREAT | 0600);
    // IPC_PRIVATE - osigurava da je segment privatan za ovaj proced i njegovu djecu
    // sizeof... - specificira velicinu segmenta
    /* IPC_CREAT | 0600 - stvara segment ako ne postoji. 0600 - dozvola
       da vlasnik moze citati i pisati */

    // Povezivanje segmenta s adresnim prostorom procesa. Vraca pokazivac (void*)
    shm = (struct shared_data *)shmat(shmid, NULL, 0);
    // shmid - id segmenta  // NULL - sustav bira adresu (nema posebnih zahtjeva)
    // 0 - standardno povezivanje tj. bez opcija

    if (shm == (void *)-1 || shmid == -1) {
        perror("greska u memoriji");
        exit(1);
    }

    // Inicijalizacija zajednickih varijabli
    shm->pravo = 0;
    shm->zastavica[0] = 0;
    shm->zastavica[1] = 0;

    pid_t pid1 = fork();
    if (pid1 == 0) proces(0);

    pid_t pid2 = fork();
    if (pid2 == 0) proces(1);

    // Roditelj ceka da oba djeteta zavrse
    for (int i = 0; i < 2; i++) {
        wait(NULL);
    }

    // Ciscenje zajednicke memorije
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);

    printf("Program zavrsen.\n");
    return 0;
}
~
~
~
~
~
~
~
~
~
~
~
~
~
~
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        1,13          All
