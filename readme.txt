Tema 2 protocoale de comunicare

Aplicatie client-server TCP si UDP pentru gestionarea mesajelor


Fisiere sursa:
- server.c - implementarea serverului
- subscriber.c - implementarea clientului
- subscribers.h
- helpers.h - implementare structuri de date auxiliare
- Makefile


## Rulare

Rulare server:
./server <PORT>

Rulare client:
./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>

Alternativ, pentru testare, puteti apela:
make run_server - va rula serverul pe portul 3030

make run_subscriber - va rula clientul cu id-ul 1234, pe portul 3030 si ip-ul 127.0.0.1


## Implementare

### Server

Serverul contine o lista de abonati si intial 3 sockets:
- pentru ascultarea de conexiuni
- pentru citirea de la tastatura
- pentru primirea mesajelor udp

La primirea unei conexiuni, va crea un client nou si un socket pentru
acesta (vezi structura client din helpers.h pentru mai multe detalii).

Se va astepta un prim mesaj de la client pentru inregistrarea id-ului.
In cazul in care id-ul este in folosinta, va inchide conexiunea si va
afisa un mesaj de eroare.
Daca id-ul este al unui client inactiv, clientul creat la conectare va 
fi sters si vor fi updatate datele clientului cu id-ul respectiv(fd, activ).
De asemenea, se vor trimite toate mesajele salvate pentru clientul respectiv.

Mesajul de la un client poate fi de 3 tipuri:
- register id - explicat mai sus 
- subscribe topic SF - se va adauga topicul in lista clientului
- unsubscribe topic - se va sterge topicul din lista clientului

In cazul in care serverul primeste un mesaj pe conexiunea upd, va
transmite mesajul tuturor clientilor activi abonati la topic. In
cazul in care clientul este offline, se va salva mesajul in listele
clientilor cu campul SF activ.

In cazul in care serverul primeste un mesaj de la tastatura, daca 
mesajul este "exit", se vor inchide toate conexiunile si se va iesi
Alternativ se va afisa la stderr mesajul.

### Client

Clientul va crea 2 socketi:
- unul pentru conexiunea cu serverul
- unul pentru primirea comenzilor de la tastatura

La conectare, se va trimite id-ul catre server. Daca id-ul este deja
in folosinta, se va inchide conexiunea.

Dupa conectare, se va astepta un mesaj de la tastatura. 

Daca mesajul este "exit", se va inchide conexiunea si se va iesi.

Daca mesajul este "subscribe topic SF", se va trimite mesajul catre
server. La fel pentru "unsubscribe".

In acelasi timp, serverul asculta pentru mesaje de la server. Daca
primeste un mesaj, il va afisa la stdout.





