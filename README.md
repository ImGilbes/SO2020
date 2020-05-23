# Organizzazione del codice in file
Il progetto e' strutturato nel seguente modo:
-	`src/`: contiene le sorgenti dei programmi del progetto, in particolare
	-	`itoa.h/.c`: dichiarazione ed implementazione della funzione che produce la rappresentazione in formato stringa di un numero intero
	-	`list.h/.c`: dichiarazione ed implementazioni di strutture dati e funzioni per la gestione di una linked-list ed di iteratori su di essi
	-	`fs.h/.c`: dichiarazione ed implementazione di funzioni per l'accesso al file system (ottenere lista di file contenuti in una directory, verificare che un file e' una directory)
	-	`file_analysis.h/.c`: dichiarazione ed implementazione di strutture dati e funzioni per la gestione di file, caratteri ed occorrenze (parsing di stringhe nel formato `path-del-file:carattere-in-decimale:numero-occorrenze`)
	-	`reader.c`: implementazione del Reader (aka Q)
	-	`slicer.c`: impementazione dello Slicer (aka P)
	-	`partitioner.c`: implementazione del Partitioner (aka C)
	-	`analyzer.c`: implementazione dell'Analyzer (aka A)
	-	`report.c`: implementazione del Report (aka R)
	-	`shell.c`: implementazione della Shell (aka M)
-	`assets/`: contiene una gerarchia di file di testo rigorosamente ASCII
-	`Makefile`: contiene le ricette per la compilazione, l'esecuzione e la verifica di correttezza dei programmi realizzati, e la pulizia della directory

# Architettura
Il progetto e' stato realizzato come un insieme di programmi indipendenti che comunicano tra loro per realizzare quanto richiesto dalla consegna. Si e' optato per utilizzare principalmente le `pipe` come tecnica di IPC, e di utilizzare i `thread` ed i `mutex` per parallelizzare la computazione di ogni processo garantendo l'accesso mutuamente esclusivo dei thread alle risorse condivise.

## Shell (aka M) <!-- TODO -->
Interagisce con l'utente per lanciare i sottosistemi Report ed Analyzer utilizzando i parametri acquisiti al lancio ed alla interazione.

Esempio:
```
> ./shell -n 2 -m 7 directory/
Sistema di analisi statistiche semplici su caratteri presenti in uno o piu' file
Comandi disponibili:
	- get $parametro
	- set $parametro $valore|default
	- list
	- add $lista_risorse
	- del $lista_risorse
	- analyze
	- report
Parametri:
	- n (default: 3)
	- m (default: 4)
> del *
> set n 5
> get n
5
> set m default
> get m
4
> add file?.txt directory/
> list
file1.txt file2.txt directory/
> analyze
> report
```

## Report (aka R) <!-- TODO -->
Report esegue delle statitiche ricevuto in input.
Dovrebbe ricevere in qualche modo dati da un processo Analyzer separato.

Esempio chiamata e di output da linea di comando:
```sh
>./report
> file1.txt:65:27
> file1.txt:66:393
> file2.txt:65:42
file1.txt
	totale caratteri: 420
	caratteri 0x65 ('A'): 27 (6.43%)
	caratteri 0x66 ('B'): 393 (93.57%)

file2.txt
	totale caratteri: 42
	caratteri 0x65 ('A'): 42 (100%)

file1.txt file2.txt:
	totale caratteri: 462
	caratteri 0x65 ('A'): 69 (14.95%)
	caratteri 0x66 ('B'): 393 (85.06%)
```

Esempio chiamata in modalita' server:
```sh
>./report
avvia Analyzer con parametri "-r 489" per comunicare con questo processo.
```

## Analyzer (aka A)
Analyzer e' il programma di avvio delle analisi, che durante l'esecuzione viene "trasformato" in un processo Partitioner utilizzando i parametri che gli erano stati passati.

L'esistenza di questo programma e' giustificata, oltre che dalle richieste del progetto, dal fatto che su richiesta dell'utente, tramite passaggio di parametri, sostituisce la stampa su console dell'output con la scrittura su `fifo`, consentendo quindi di indirizzare i risultati delle analisi ad un altro processo (il processo Analyzer).

Il sistema utilizza quindi `dup2` ed `execve`.

Esempio chiamata e di output da linea di comando:
```sh
>./analyzer -n 3 -m 4 file1.txt file2.txt
file1.txt:65:27
file1.txt:66:393
file2.txt:65:42
```

Esempio chiamata comunicandogli il pid di Report, quindi riportando ad esso i risultati delle analisi:
```sh
>./analyzer -r 489 -n 3 -m 4 file1.txt file2.txt
```

## Partitioner (aka C)
Partitioner e' il programma che riceve la lista di file da analizzare, parallelizzando la lettura di essi in n processi Slicer, ogniuno dei quali riceve un partizione dei file.

Il thread principale avvia un numero di processi Slicer pari al numero di partizioni in cui dividere i file. Questi funzionano esattamente come descritto nella sezione Slicer, meno il fatto che il loro output e' scritto in una pipe specifica per il processo in questione. Inoltre, per ogni processo, viene avviato anche un thread di ascolto, che e' adibito alla lettura della pipe in cui un processo Slicer scrive.

I thread di ascolto, dato che ricevono informazioni "complete" e che non necessitano di computazioni su di essi, stampano direttamente in console le informazioni che ricevono.

Il sistema utilizza quindi `fork`, `execve`, `pipe`, `dup2`, `pthread_create`, `pthread_join` e `pthread_mutex_t`. Le prime quattro per avvia processi Slicer ed instaurare una comunicazione tra essi e il Partitioner, le ultime per gestire i thread di ascolto e stampare le informazioni ricevute, senza memorizzarle dato che non richiedono di essere processate, garantendo la mutua esclusione all'accesso allo standard output.

Esempio chiamata e di output da linea di comando:
```sh
>./partitioner -n 3 -m 4 file1.txt file2.txt # lettura dei file file1.txt e file2.txt, in un partizionamento da 3 partizioni (quindi composto da {{file1.txt}, {file2.txt}, {}})
file1.txt:65:27		# file1.txt: il carattere ASCII 65 ('A') appare 27 volte (file1.txt della partizione {file1.txt})
file1.txt:66:393	# file1.txt: il carattere ASCII 6B ('B') appare 393 volte (file1.txt della partizione {file1.txt})
file2.txt:65:42		# file2.txt: il carattere ASCII 65 ('A') appare 42 volte (file2.txt della partizione {file2.txt})
```

## Slicer (aka P)
Slicer e' il programma che esegue l'analisi sulla partizione di file ricevuta, parallelizzando la lettura in m processi Reader, ogniuno con la propria slice.

Il thread principale avvia un numero di processi Reader pari al numero di slice di cui i file sono composti. Questi funzionano esattamente come descritto nella sezione Reader, meno il fatto che il loro output e' scritto in una pipe specifica per il processo in questione. Inoltre, per ogni processo, viene avviato anche un thread di ascolto, che e' adibito alla lettura della pipe in cui un processo Reader scrive.

Una volta terminati i thread di ascolto, il programma stampa il resoconto delle occorrenze di caratteri per ogni file di tutte le sue slice, quindi del file intero.

Il sistema utilizza quindi `fork`, `execve`, `pipe`, `dup2`, `pthread_create`, `pthread_join` e `pthread_mutex_t`. Le prime quattro per avvia processi Reader ed instaurare una comunicazione tra essi e lo Slicer, le ultime per gestire i thread di ascolto ed aggiornare le occorrenze garentendo la mutua esclusione all'accesso della sezione critica.

Esempio chiamata e di output da linea di comando:
```sh
>./slicer -m 4 file1.txt file2.txt  # lettura dei file file1.txt e file2.txt, ogniuno visto come composto di 4 slice
file1.txt:65:108    # file1.txt: il carattere ASCII 65 ('A') appare 108 volte (in tutte le sue slice)
file1.txt:66:1572   # file1.txt: il carattere ASCII 66 ('B') appare 1572 volte (in tutte le sue slice)
file2.txt:65:260    # file2.txt: il carattere ASCII 65 ('A') appare 260 volte (in tutte le sue slice)
file2.txt:66:16     # file2.txt: il carattere ASCII 66 ('B') appare 16 volte (in tutte le sue slice)
```

## Reader (aka Q)
Reader e' il programma che legge una specifica slice di ogni file indicatogli, parallelizzando la lettura tra diversi thread.

Il thread principale avvia un numero di thread di lettura pari al numero di file ricevuti alla chiamata. Ogniuno di questi legge uno dei file in questione nella sola slice che gli compete, aggiornando le occorrenze dei caratteri *in quel file*.

Quando tutti i thread di lettura delle slice sono terminati, il thread principale stampa in su console il resoconto delle occorrenze dei caratteri rilevate. Occorrenze pari a 0 sono ignorate.

Il sistema utilizza esclusivamente `pthread_create` e `pthread_join`, senza utilizzare tecniche si sincronizzazione, dato che ogni thread di lettura, pur condividendo lo stesso spazio di indirizzamento, accedere ad un'area di memoria a lui esclusiva durante la sua esecuzione.

Esempio chiamata e di output da linea di comando:
```sh
>./reader -s 2 -m 4 file1.txt file2.txt # lettura della slice 2 di 4 dei file file1.txt e file2.txt
file1.txt:65:27     # file1.txt: il carattere ASCII 65 ('A') appare 27 volte (nella slice 2/4)
file1.txt:66:393    # file1.txt: il carattere ASCII 66 ('B') appare 393 volte (nella slice 2/4)
file2.txt:65:65     # file2.txt: il carattere ASCII 65 ('A') appare 65 volte (nella slice 2/4)
file2.txt:66:4      # file2.txt: il carattere ASCII 66 ('B') appare 4 volte (nella slice 2/4)
```