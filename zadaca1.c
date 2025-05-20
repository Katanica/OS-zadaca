#include <stdio.h>  // standardna biblioteka za fprint...
#include <signal.h> // biblioteka za signale (SIGINT...)
#include <stdlib.h> // biblioteka za exit
#include <unistd.h> // biblioteka za sleep, fork...
#include <math.h>   // biblioteka za sqrt

/* volatile osigurava da se varijabla uvijek cita iz memorije a ne iz cache-a.
   Koristimo volatile da bi promjene iz singal obrada bile vidljive */
volatile int trenutni_broj = 0;  // varijabla u koju se pohranjuje trenutni broj sa kojim se radi
const char *obrada_file = "obrada.txt"; // datoteka u koju upisujemo sve brojeve
const char *status_file = "status.txt"; // datotetka koja cuva trenutni status programa

void obradi_sigusr1(int sig) {  // FUNKCIJA KOJA ISPISUJE TRENUTNI BROJ
    printf("Trenutni broj: %d\n", trenutni_broj);
    fflush(stdout); // ciscenje buffera
}

void obradi_sigterm(int sig) {  // ZAUSTAVLJA PROCES I ZAPISUJE BROJ KOJI JE BIO U OBRADI
    FILE *fp = fopen(status_file, "w"); // otvara datetoku u write nacinu
    if (fp == NULL) {  // u slucaju da je nesto poslo krivo
        fprintf(stderr, "Ne mogu otvoriti %s\n", status_file);
    }
    fprintf(fp, "%d\n", trenutni_broj);  // ispisi trenutni broj u fp datoteku
    fclose(fp);
    exit(0);
}

void obradi_sigint(int sig) {  // SAMO IZLAZI IZ PROCESA
    exit(0);
}

int procitaj_status() { /* procitaj broj u statusu: 0 - kreni od pocetka
                                                    >0 - kreni od tog broja */
    FILE *fp = fopen(status_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Ne mogu otvoriti %s\n", status_file);
        exit(1);
    }
    int broj;
    if (fscanf(fp, "%d", &broj) != 1) { // ako nije uspjesno procitan broj iz fp
        broj = 0;                       // vrati 0
    }
    fclose(fp);
    return broj;
}

int obrada_zadnji_broj() {  // funkcija koja pronalazi zadnji broj u obrada.txt
    FILE *fp = fopen(obrada_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Ne mogu otvoriti %s\n", obrada_file);
        exit(1);
    }
    int broj;
    int zadnji_kvadrat = 0;
    while (fscanf(fp, "%d", &broj) == 1) {  // citamo brojeve sve dok ih ima
        zadnji_kvadrat = broj;     // varijabla zadnji kvadrat "kasni" u citanju brojeva
    }
    fclose(fp);
    return (int)sqrt((double)zadnji_kvadrat);   // sqrt ocekzje double kao ulaz
}

void zapisi_u_obradu(int kvadrat) {
    FILE *fp = fopen(obrada_file, "a"); // a - append / pisi na kraj
    if (fp == NULL) {
        fprintf(stderr, "Ne mogu otvoriti %s\n", obrada_file);
        exit(1);
    }
    fprintf(fp, "%d\n", kvadrat); // zapisi u fp, kvadrate, svaki u novi red
    fclose(fp);
}

void zapisi_u_status(int broj) { // zapisi u status
    FILE *fp = fopen(status_file, "w");
    if (fp == NULL) {
        fprintf(stderr, "Ne mogu otvoriti %s\n", status_file);
        exit(1);
    }
    fprintf(fp, "%d", broj);
    fclose(fp);
}

int main() {
    // Postavljanje signala
    signal(SIGUSR1, obradi_sigusr1);
    signal(SIGTERM, obradi_sigterm);
    signal(SIGINT, obradi_sigint);

    // Čitanje statusa za slucaj sigterm
    trenutni_broj = procitaj_status();

    // Ako je status 0, analiziraj obrada.txt
    // za slucaj sigint
    if (trenutni_broj == 0) {
        trenutni_broj = obrada_zadnji_broj();
    }

    // Zapiši 0 u status.txt
    zapisi_u_status(0);

    // Beskonačna petlja
    while (1) {
        trenutni_broj++;
        int kvadrat = trenutni_broj * trenutni_broj;
        zapisi_u_obradu(kvadrat);
        int preostalo = 5;
        while(preostalo > 0)
                preostalo = sleep(preostalo);
    }

    return 0;
}
