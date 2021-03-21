Nume: STIUCA Roxana-Elena
Grupa: 335CB

# Tema 1 SO - Multi-platform Development - C-Preprocessor

### Organizare
Programul are nevoie de urmatoarele structuri:
- un hashmap care retine maparile simbolurilor oferite fie prin argumentul -D,
fie prin directiva #define; pt. acesta, sunt oferite operatiile de baza:
new, insert, update, delete, free.
- un vector de nume de foldere in care se pot regasi fisierele header incluse.
- un file pointer catre fisierul (initial) de input sau stdin in absenta
acestuia.
- un file pointer catre fisierul de output sau stdout in absenta acestuia.

Etapele programului:
- **init**: parsarea argumentelor din linia de comanda si initializarea
structurilor necesare.
- **preprocess_file**: parsarea inputului si rezolvarea directivelor; pt.
directive conditionale sau pt. #include, functia se va auto-apela. Adica
pt. un input ca acesta:
```
1	#if CEVA
2		#if ALTCEVA1
3		...
4		#elif ALTCEVA2
5			#include "file.h"
6		#else
7		...
8	#else
9	...
```
liniile 1,8,9 apartin unui apel al functiei; liniile 2,3,4,6,7 apartin
unui apel suplimentar (in care conditition depinde de ALTCEVA1,ALTCEVA2);
linia 6 si tot ce exista in file.h apartin unui alt apel suplimentar.
Acest lucru asigura corectitudinea programului in cazuri de directive
conditionale sau de include-uri imbricate (chiar daca , cazul de if in if
nu apare in teste).
- **end_program**: dezalocari finale si inchiderea fisierelor.

Functiile pt. operatii pe stringuri se gasesc in utils.c/.h.
Definitia si implementarea structurii de hashmap se gaseste in map.c/.h.

Programul trateaza erori precum ENOMEM, EINVAL, ENOENT, propagand eroarea
pana in main si eliberand toate resursele indiferent daca programul se termina
normal sau cu eroare.

### Implementare
Este implementat intreg enuntul.

#### Implementare "hashmap"
Abordarea nu foloseste o functie de hash si are complexitatea O(N) pt.
inserare, stergere, gasire.
Este implementat ca o lista simplu inaltuita. Inserarea se face la finalul
listei. O imbunatatire ar fi inserarea la inceput (O(1)), dar trebuie asigurata
unicitatea simbolului in lista (deci oricum trebuie parcursa lista pt. a ne
asigura ca nu exista deja).

#### Implementare pt. parsarea si procesarea inputului
Inputul este citit linie cu linie. Fiecare linie este sparta in
**tokeni/cuvinte**.
Sunt inlocuite simbolurile cu maparile lor din hashmap. Exista exceptii
cand inlocuirea nu are loc (ex. #define SYM 1 #ifdef SYM nu devine #ifdef 1 sau
"SYM" nu devine "1").
Daca primul cuvant corespunde unei directive, aceasta este rezolvata, daca
condition nu este pe 0 (in acest caz ne aflam intr-o sectiune de cod sub
o directiva conditionala care s-a evaluat pe false; deci codul de sub nu se
evalueaza). Daca nu este o directiva, afisam linia (daca condition nu e 0).

### Cum se compileaza si cum se ruleaza?
- **Compilare**: Linux - make / make build; Windows - nmake.
- **Rulare**:
`./so-cpp [-D <SYMBOL>[=<MAPPING>]] [-I <DIR>] [<INFILE>] [[-o] <OUTFILE>]`

### Observatii finale
Am invatat foarte putine despre SO. 95% din tema a fost procesare de stringuri
in C.