#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define N 5 // Broj filozofa

// Zajednički resursi
char filozof[N]; // Stanje svakog filozofa: 'O' (razmišlja), 'o' (čeka), 'X' (jede)
int stapic[N];  // Dostupnost štapića: 1 (dostupan), 0 (zauzet)
pthread_cond_t red[N]; // Uvjetne varijable za svakog filozofa
// ^^ stavlja filozofa na čekanje dok štapići ne budu slobodni
pthread_mutex_t monitor; // Mutex za monitor

// Funkcija za ispis stanja svih filozofa
void ispiši_stanje(int n) {
    printf("Stanje filozofa: ");
    for (int i = 0; i < N; i++) {
        printf("%c ", filozof[i]);
    }
    printf("(Filozof %d)\n", n + 1);
}

// Funkcija monitora za ulazak u kritični odsječak i uzimanje štapića
void udi_u_monitor(int n) {
    pthread_mutex_lock(&monitor);   // "zakljucava" mutex
    filozof[n] = 'o'; // Filozof čeka
    // Čeka dok oba štapića nisu dostupna
    while (stapic[n] == 0 || stapic[(n + 1) % N] == 0) {
        pthread_cond_wait(&red[n], &monitor);
    }
    // Uzima oba štapića
    stapic[n] = 0;
    stapic[(n + 1) % N] = 0;
    filozof[n] = 'X'; // Filozof jede
    ispiši_stanje(n);
    pthread_mutex_unlock(&monitor);
}

// Funkcija monitora za oslobađanje štapića i izlazak iz kritičnog odsječka
void izadi_iz_monitora(int n) {
    pthread_mutex_lock(&monitor);
    filozof[n] = 'O'; // Filozof razmišlja
    stapic[n] = 1; // Oslobađa lijevi štapić
    stapic[(n + 1) % N] = 1; // Oslobađa desni štapić
    // Signalizira susjedne filozofe koji možda čekaju
    pthread_cond_signal(&red[(n - 1 + N) % N]);
    pthread_cond_signal(&red[(n + 1) % N]);
    ispiši_stanje(n);
    pthread_mutex_unlock(&monitor);
}

// Simulacija razmišljanja
void misliti(int n) {
    printf("Filozof %d misli...\n", n + 1);
    sleep(3); // Simulira vrijeme razmišljanja
}

// Simulacija jela
void jesti(int n) {
    udi_u_monitor(n);
    printf("Filozof %d jede...\n", n + 1);
    sleep(2); // Simulira vrijeme jela
    izadi_iz_monitora(n);
}

// Funkcija dretve za svakog filozofa. Funkcija dretve pa ima *
    // void *arg - prosljedivanje podataka dretvi
    // (int)(long)arg - vracanje u prvobitni oblik
void *filozof_thread(void *arg) {
    int n = (int)(long)arg; // ID filozofa
    while (1) {
        misliti(n);
        jesti(n);
    }
    return NULL;
}

int main() {
    pthread_t filozofi[N];  // polje dretvi
    
    // Inicijalizacija zajedničkih resursa
    for (int i = 0; i < N; i++) {
        filozof[i] = 'O'; // Svi filozofi počinju razmišljati
        stapic[i] = 1;    // Svi štapići su dostupni
        pthread_cond_init(&red[i], NULL);   // inicijaliziranje uvjetne varijable red[i]
    }
    pthread_mutex_init(&monitor, NULL); // inicijaliziranje mutex-a

    // Kreiranje dretvi filozofa
    for (int i = 0; i < N; i++) {                               
        if (pthread_create(&filozofi[i], NULL, filozof_thread, (void *)(long)i)) {  
            // &filozofi[i] - pokazivač gdje se sprema ID nove dretve
            // NULL - zadane postavke
            // filozof_thread - pozivanje funkcije
            // (void *)(long)i - prosljeduje i funkciji filozof_thread
            printf("Greška pri kreiranju dretve filozofa %d!\n", i + 1);
            return 1;
        }
    }
    
    // Čekanje na dretve 
    for (int i = 0; i < N; i++) {
        pthread_join(filozofi[i], NULL);
    }

    // Čišćenje (nedostižno zbog beskonačne petlje)
    for (int i = 0; i < N; i++) {
        pthread_cond_destroy(&red[i]);
    }
    pthread_mutex_destroy(&monitor);

    return 0;
}