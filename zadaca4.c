#include <stdio.h>      // stadnardna biblioteka (printf)
#include <stdlib.h>     // dinamičko upravljanje memorijom (exit)
#include <errno.h>      // koristi se za provjeru grešaka sustava
#include <sys/types.h>  // tipovi podataka potrebni za rad s procesima i IPC-om
#include <sys/ipc.h>    // definicije za međuprocesnu komunikaciju (IPC)
#include <sys/sem.h>    // definicije za rad sa System V semaforima (semget, semop, semctl)   // JER KORISTIMO PROCESE  
#include <sys/shm.h>    // funkcije za rad za zajednickom memorijom (shmget, shmat) 
#include <unistd.h>     // POSIX funkcije (fork, sleep)
#include <sys/wait.h>   // omogucuje koristenje wait (čekanje završetka dječjih procesa)

#define N 5 // Broj sjedala na vrtuljku
#define POSJETITELJI 15 // Broj procesa posjetitelja-

union unija {   // unija koju koristi semctl za upravljanje semaforima
    int val;    // postavljanje pojedinačne vrijednosti semafora (inicijalizacija)
};  

// funkcija koja izvodi operacije nad semaforima
void sem_op(int semid, int sem_num, int op) {   
    // semid - id SKUPA semafora; sem_num - indeks SPECIFICNOG semafora unutar skupa
    // op - operacija na semaforu (-1 - dekrementiranje, +1 - inkrementiranje)
    
    // struktura koja specificira operaciju
    struct sembuf sb = {sem_num, op, 0};
    // sem_num - indeks semafora u skupu
    // op - vrijednost za promjenu semafora (>0 - povećanje, <0 - smanjenje) 
    // 0 - zastavice
    if (semop(semid, &sb, 1) == -1) { // poziva semop za izvršenje operacije na semaforu. OČEKUJE STRUKTURU KAO PARAMETAR
        perror("Greška funkcije: semop");
        exit(1);
    }
}
// funkcija koja simulira ponašanje posjetitelja (jednog procesa)
void posjetitelj(int id, int semid, int *sjeli) {
    // id - id posjetitelja;
    sem_op(semid, 1, -1); // -1 posjetitelj moze cekati ukrcavanje (poč. vrijednost = N)
    sem_op(semid, 0, -1); // Uzima mutex
    (*sjeli)++;    // uvecava broj onih koji su sjeli
    printf("Posjetitelj %d: Ušao (sjedala: %d/%d)\n", id, *sjeli, N);
    if (*sjeli == N) {
        printf("VRTULJAK PUN. POČINJE VOŽNJA\n");
        sem_op(semid, 3, 1); // Signal svi_ukrcani (0 ili 1)
    }
    sem_op(semid, 0, 1); // Otpusti mutex
    sem_op(semid, 2, -1); // Čeka kraj vožnje
    sem_op(semid, 0, -1); // Uzima mutex
    (*sjeli)--;
    printf("Posjetitelj %d: Sišao (sjedala: %d/%d)\n", id, *sjeli, N);
    if (*sjeli == 0) {
        printf("SVI SIŠLI. SLIJEDI NOVO UKRCAVANJE\n");
        sem_op(semid, 1, N); // Ponovo može ući N posjetitelja
    }
    sem_op(semid, 0, 1); // Otpusti mutex
}

void vrtuljak(int semid) {
    while (1) {
        printf("Vrtuljak: Čeka %d posjetitelja\n", N);
        sem_op(semid, 3, -1); // smanji svi_ukrcani za 1 i pokrece vožnju
        printf("Vrtuljak: Počinje vožnja s %d posjetitelja\n", N);
        sleep(3); // Simulira vožnju
        printf("Vrtuljak: Vožnja završena\n");
        sem_op(semid, 2, N); // Signal red_iskrcavanja
    }
}

int main() {
    key_t key = 1234;      // ključ za IPC (semafori i zajednička memorija)
    int semid, shmid;   // identifikator skupa semafora, // identifikator zajedničke memorije
    int *sjeli;    // pokazivač na zajedničku memoriju za brojanje posljetitelja na vrtuljku
    pid_t pid;      // identifikator procesa za fork
    union unija kontrola; // unija za upravljanje semaforima
    
    // semid - identifikator skupa semafora
    // 4 - stvara skup od 4 semafora; 0666 - pravo čitanja i pisanja za sve
    // IPC_CREAT - stvori ako već ne postoji 
    if ((semid = semget(key, 4, 0666 | IPC_CREAT)) == -1) {
        perror("Greška: semget");
        exit(1);
    }
    // SEMCTL - UPRAVLJA SEMAFORIMA
    // SETVAL - OPERACIJA POSTAVLJANJA VRIJEDNOSTI
    // KONTROLA.VAL - POSTAVLJA POČ. VRIJEDNOST SEMAFORA
    kontrola.val = 1; 
    // 0 - osigurava da jedan proces mijenja zajednicku memoriju (sjeli)
    if (semctl(semid, 0, SETVAL, kontrola) == -1) {   // mutex
        perror("Greška: semctl mutex");
        exit(1);
    }
    kontrola.val = N; 
    // 1 - kontrolira koliko posjetitelja moze cekati na ukrcavanje
    if (semctl(semid, 1, SETVAL, kontrola) == -1) {
        perror("semctl redUkrcavanja");
        exit(1);
    }
    kontrola.val = 0; // 2 - posjetitelji mogu sići
    if (semctl(semid, 2, SETVAL, kontrola) == -1) {
        perror("semctl redIskrcavanja");
        exit(1);
    }
    kontrola.val = 0; // 3 - svi su ukrcani
    if (semctl(semid, 3, SETVAL, kontrola) == -1) {
        perror("semctl sviUkrcani");
        exit(1);
    }
    // id dijeljene memorije
    if ((shmid = shmget(key, sizeof(int), 0666 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
    
    // povezujemo sa dijeljenom memorijom kako bi svi procesi mogli pristupiti
    sjeli = (int *)shmat(shmid, NULL, 0);
    if (sjeli == (int *)-1) {
        perror("shmat");
        exit(1);
    }
    *sjeli = 0;

    pid = fork();   // pokrecemo vrtuljak
    if (pid == -1) {
        perror("vrtuljak fork");
        exit(1);
    }
    if (pid == 0) {
        vrtuljak(semid);
        exit(0);
    }
    // stvaramo procese / posjetitelje
    for (int i = 0; i < POSJETITELJI; i++) {
        pid = fork();
        if (pid == -1) {
            perror("posjetitelj fork");
            exit(1);
        }
        if (pid == 0) {
            posjetitelj(i + 1, semid, sjeli);
            exit(0);
        }
    }

    for (int i = 0; i < POSJETITELJI + 1; i++) {
        wait(NULL);
    }

    shmdt(sjeli);
    // IPC_RMID - naredba za uklanjanje zajednicke memorije ili skupa semafora
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    printf("Main: Čišćenje završeno\n");
    return 0;
}
