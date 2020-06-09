# Organizzazione del codice in file
Il progetto e' strutturato nel seguente modo:
-	`src/`: contiene le sorgenti dei programmi del progetto, in particolare
	-	`itoa.h/.c`: dichiarazione ed implementazione della funzione che produce la rappresentazione in formato stringa di un numero intero
	-	`list.h/.c`: dichiarazione ed implementazioni di strutture dati e funzioni per la gestione di una linked-list ed di iteratori su di essi
	-	`fs.h/.c`: dichiarazione ed implementazione di funzioni per l'accesso al file system (ottenere lista di file contenuti in una directory, verificare che un file e' una directory)
	-	`file_analysis.h/.c`: dichiarazione ed implementazione di strutture dati e funzioni per la gestione di file, caratteri ed occorrenze (parsing di stringhe nel formato `path-del-file:carattere-in-decimale:numero-occorrenze`)
	-	`settings.h`: definizione di macro per impostare le dimensioni di alcune strutture dati utilizzate
	-	`bool.h`: implementazione del tipo e dei valori booleani
	-	`utilities.h`: dichiarazione e implementazione di funzioni utili
	-	`history.h/.c`: dichiarazione ed implementazione di una struttura dati per memorizzare i risultati delle analisi all'interno della shell
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

Le `fifo` sono state utilizzate solo in un caso, per mettere in comunicazione 2 processi che non hanno una gerarchia diretta (nello scenario di metterli in comunicazione quando sono stati avviati in due terminali separati). Nello scenario in cui ci sia una gerarchia diretta, sono comunque utilizzate le `pipe`.

## Reader (aka Q)
Reader e' il programma che legge una specifica slice di ogni file indicatogli, parallelizzando la lettura tra diversi thread.

Il thread principale avvia un numero di thread di lettura pari al numero di file ricevuti alla chiamata. Ogniuno di questi legge uno dei file in questione nella sola slice che gli compete, aggiornando le occorrenze dei caratteri *in quel file*. Nel caso un file non esista, l'esecuzione e *quiet*, e non viene riportato alcun errore (l'esecuzione procede come se non fosse stato specificato il file).

Quando tutti i thread di lettura delle slice sono terminati, il thread principale stampa in console il resoconto delle occorrenze dei caratteri rilevati. Occorrenze pari a 0 sono ignorate.

Il sistema utilizza esclusivamente `pthread_create` e `pthread_join`, senza utilizzare tecniche si sincronizzazione, dato che ogni thread di lettura, pur condividendo lo stesso spazio di indirizzamento, accedere ad un'area di memoria a lui esclusiva durante la sua esecuzione.

Esempio chiamata e di output da linea di comando:
```sh
>bin/reader -s 2 -m 4 file1.txt file2.txt # lettura della slice 2 di 4 dei file file1.txt e file2.txt
file1.txt:65:27     # file1.txt: il carattere ASCII 65 ('A') appare 27 volte (nella slice 2/4)
file1.txt:66:393    # file1.txt: il carattere ASCII 66 ('B') appare 393 volte (nella slice 2/4)
file2.txt:65:65     # file2.txt: il carattere ASCII 65 ('A') appare 65 volte (nella slice 2/4)
file2.txt:66:4      # file2.txt: il carattere ASCII 66 ('B') appare 4 volte (nella slice 2/4)
```

### Parametri (opzionali)
- `-m`: la stringa seguente identifica il numero di slice in cui il file e' suddiviso (di default e' 1)
- `-s`: la stringa seguente identifica il numero della slice da leggere (di default e' 1)

## Slicer (aka P)
Slicer e' il programma che esegue l'analisi sulla partizione di file ricevuta, parallelizzando la lettura in *m* processi Reader, ogniuno con la propria slice.

Il thread principale avvia un numero di processi Reader pari al numero di slice di cui i file sono composti. Questi funzionano esattamente come descritto nella sezione Reader, meno il fatto che il loro output e' scritto in una pipe specifica per il processo in questione. Inoltre, per ogni processo, viene avviato anche un thread di ascolto, che e' adibito alla lettura della pipe in cui un processo Reader scrive.

Una volta terminati i thread di ascolto, il programma stampa il resoconto delle occorrenze di caratteri per ogni file di tutte le sue slice, quindi del file intero.

Il sistema utilizza quindi `fork`, `execve`, `pipe`, `dup2`, `pthread_create`, `pthread_join` e `pthread_mutex_t`. Le prime quattro per avviare processi Reader ed instaurare una comunicazione tra essi e lo Slicer, le ultime per gestire i thread di ascolto ed aggiornare le occorrenze garentendo la mutua esclusione all'accesso della sezione critica.

Esempio chiamata e di output da linea di comando:
```sh
>bin/slicer -m 4 file1.txt file2.txt  # lettura dei file file1.txt e file2.txt, ogniuno visto come composto di 4 slice
file1.txt:65:108    # file1.txt: il carattere ASCII 65 ('A') appare 108 volte (in tutte le sue slice)
file1.txt:66:1572   # file1.txt: il carattere ASCII 66 ('B') appare 1572 volte (in tutte le sue slice)
file2.txt:65:260    # file2.txt: il carattere ASCII 65 ('A') appare 260 volte (in tutte le sue slice)
file2.txt:66:16     # file2.txt: il carattere ASCII 66 ('B') appare 16 volte (in tutte le sue slice)
```

### Parametri (opzionali)
- `-m`: la stringa seguente identifica il numero di slice in cui il file e' suddiviso (di default e' 4)

## Partitioner (aka C)
Partitioner e' il programma che riceve la lista di file da analizzare, parallelizzando la lettura di essi in *n* processi Slicer, ogniuno dei quali riceve un partizione dei file.

Il thread principale avvia un numero di processi Slicer pari al numero di partizioni in cui dividere i file. Questi funzionano esattamente come descritto nella sezione Slicer, meno il fatto che il loro output e' scritto in una pipe specifica per il processo in questione. Inoltre, per ogni processo, viene avviato anche un thread di ascolto, che e' adibito alla lettura della pipe in cui un processo Slicer scrive. Nel caso di partizioni vuote (nel caso sia stato richiesto un numero di partizioni maggiore del numero di file effettivi), processi e thread vengono comunque avviati, ma sono "silenziosi".

I thread di ascolto, dato che ricevono informazioni "complete" e che non necessitano di computazioni su di essi, stampano direttamente in console le informazioni man mano che le ricevono.

Il sistema utilizza quindi `fork`, `execve`, `pipe`, `dup2`, `pthread_create`, `pthread_join` e `pthread_mutex_t`. Le prime quattro per avvia processi Slicer ed instaurare una comunicazione tra essi e il Partitioner, le ultime per gestire i thread di ascolto e stampare le informazioni ricevute, senza memorizzarle dato che non richiedono di essere processate, garantendo la mutua esclusione all'accesso allo standard output.

Esempio chiamata e di output da linea di comando:
```sh
>bin/partitioner -n 3 -m 4 file1.txt file2.txt # lettura dei file file1.txt e file2.txt, in un partizionamento da 3 partizioni (quindi composto da {{file1.txt}, {file2.txt}, {}})
file1.txt:65:27		# file1.txt: il carattere ASCII 65 ('A') appare 27 volte (file1.txt della partizione {file1.txt})
file1.txt:66:393	# file1.txt: il carattere ASCII 6B ('B') appare 393 volte (file1.txt della partizione {file1.txt})
file2.txt:65:42		# file2.txt: il carattere ASCII 65 ('A') appare 42 volte (file2.txt della partizione {file2.txt})
```

### Parametri (opzionali)
- `-m`: la stringa seguente identifica il numero di slice in cui il file e' suddiviso (di default e' 4)
- `-n`: la stringa seguente identifica il numero di partitioni in cui dividere la lista dei file (di default e' 3)

## Analyzer (aka A)
Analyzer e' il programma di avvio delle analisi, che durante l'esecuzione viene "trasformato" in un processo Partitioner utilizzando i parametri che gli erano stati passati.

L'esistenza di questo programma e' giustificata, oltre che dalle richieste del progetto, per 2 risolvere esigenze:
- "espandere" le directory passategli per argomento nel loro contenuto, in modo ricorsivo
- su richiesta dell'utente, invece di riportare i risultati delle analisi in console, le inoltra ad un processo Report (descritto piu' avanti) per mezzo di una `fifo`

Il sistema utilizza quindi `dup2` ed `execve`.

Esempio chiamata e di output da linea di comando:
```sh
>bin/analyzer -n 3 -m 4 file1.txt directory # lettura di file1.txt e, ricorsivamente, del contenuto di directory
file1.txt:65:27
file1.txt:66:393
directory/file2.txt:65:42		# e' stata "espansa" directory/
directory/sub/file3.txt:66:420  # e' stata "espansa" directory/sub/
```

Esempio chiamata comunicandogli il pid di Report, quindi riportando ad esso i risultati delle analisi:
```sh
>bin/analyzer -r -n 3 -m 4 file1.txt directory	# l'output e' inviato al processo report
```

### Parametri (opzionali)
- `-m`: la stringa seguente identifica il numero di slice in cui il file e' suddiviso (di default e' 4)
- `-n`: la stringa seguente identifica il numero di partitioni in cui dividere la lista dei file (di default e' 3)
- `-r`: invia i risultati della analisi ad un processo Report

## Report (aka R)
Report e' il programma che esegue statitiche sui file ricevute in input. Restituisce in stdout le informazioni che le sono state richieste tramite passaggio di parametri, come successivamente illustrato. Si noti che per l'utente che desidera utilizzare Report come programma stand-alone e' possibile personalizzare le stampe a video chiamando report con i parametri che preferisce.  

Una volta avviato, Report legge le righe `file:carattere:occorrenze` ricevute in stdin e le parsa per estrapolarne le informazioni. Alla ricezione di `EOF`, ottenibile anche da tastiera con `Ctrl-D`, il programma restituisce le informazioni su quanto letto sino ad allora.

Avviando il report con il parametro `npipe`, Report legge le righe non piu' da stdin, ma dalla fifo presente in `/tmp/report_fifo`, consentendo di fatto di ricevere le stringhe `file:carattere:occorrenze` da un processo `analyzer` avviato con parametro `-r`.


Esempio chiamata e di output da linea di comando:
```sh
>bin/report cmdline allchars -ls -sp -p -np -allM --allm
file1.txt:65:27
file1.txt:66:393
file2.txt:65:42
[Ctrl-D]
Spazi: 0 (0.00%)
Caratteri stampabili: 462 (100.00%)
Caratteri non stampabili: 0 (0.00%)

Lettere Maiuscole: 462 (100.00%)
caratteri 0x41 (A): 69 (14.94%)
caratteri 0x42 (B): 393 (85.06%)
```

Esempio chiamata con lettura da file:
```sh
>bin/report file allchars -ls -1 <saveFileName.extension  # legge i dati, invece che da stdin, dalla fifo localizzata in /tmp/report_fifo
Avviare l'analyzer con il parametro -r
```

Esempio chiamata in modalita' server:
```sh
>bin/report npipe allchars -ls -p -np -sp -punt -allM --allm	# legge i dati, invece che da stdin, dalla fifo localizzata in /tmp/report_fifo
Avviare l'analyzer con il parametro -r
```

## Parametri obbligatori
Alla chiamata di report, per il corretto funzionamento, si devo aggiungere due campi: 
- Il primo, parametrizzato come segue: `npipe` or `file` or `cmdline`, specifica la modalità di input del report
- Il secondo, parametrizzato con: `allchars` or `ponly`, specifica le modalita' di calcolo del totale, ovvero con ponly solo i caratteri stampabili saranno utilizzati per calcolare le percentuali riportate in seguito, con allchars saranno inclusi anche i caratteri non stampabili.

### Parametri per specificare le informazioni richieste
- `-ls`: stampa lista file e totale caratteri
- `-p`: totale caratteri stampabili
- `-np`: totale caratteri non stampabili
- `-lett`: totale lettere
- `-punt`: totale punteggiatura
- `-allpunt`: totale punteggiatura con dati per ciascun singolo carattere di punteggiatura
- `-M`: totale maiuscole
- `-allM`: totale maiuscole con dati per ogni lettera maiuscola
- `-m`: totale minuscole
- `-allm`: totale minuscole con dati per ogni lettera minuscola
- `-sp`: totale spazi
- `-num`: totale numeri
- `-allnum`: totale numeri con dati per ogni numero
- `-allch`: dati specifici per ogni carattere
Le seguenti sono parametri da utilizzare per stampe aggregate, non personalizzabili, create per semplificare le chiamate
- `-1`: resoconto generale per ogni gruppo di caratteri
- `-2`: resoconto caratteri stmapabili
- `-3`: resoconto lettere
- `-4`: resoconto di spazi, numeri, punteggiatura
NB.: nel calcolo delle parcentuali per i parametri -allch, -p, -np il totale utilizzato considerera' sempre caratteri stampabili e non stampabili insieme, indipendentemente dal parametro `allchars` o `ponly` precedentemente specificato

## Shell (aka M)
Shell e' il programma che consente di utilizzare il sistema in modo interattivo e facilitato per l'utente.

Sono implementati diversi comandi, che sono di seguito illustrati:
- `help`: stampa a video un breve riassunto dei comandi possibili
- `get $var`: restituisce il valore della variabile indicata (`m` o `n`)
- `set $var $val|default`: imposta il valore della variabile indicata (`m` o `n`) con il valore numerico (intero positivo) di `val`, oppure con il valore di default (`m` = 3, `n` = 4)
- `list`: stampa a video le risorse da analizzare
- `add file*`: aggiunge i file specificati tra le risorse da analizzare
- `del file*`: rimuove i file specificati dalle risorse da analizzare
- `analyze`: avvia l'analisi sulle risorse impostate con i valori di `m` e di `n` impostati. I risultati sono salvati internamente
- `history`: stampa a video uno storico delle analisi effettuate, visualizzando data e ora dell'esecuzione e la lista delle risorse incluse
- `report [$history_id]`: avvia il report sui dati presenti nel record di history indicato da `history_id`. Se non espresso, la esegue sull'ultima analisi effettuata
- `import $file`: importa i risultati delle analisi precedentemente esportate, assieme alla lista di risorse ed alla data ed ora di esecuzione
- `export $history_id $file`: esporta i risultati della analisi, assieme alla lista di risorse ed alla data ed ora di esecuzione
- `exit`: chiude la shell

Esempio:
```
> bin/shell
Sistema di analisi statistiche semplici su caratteri presenti in uno o piu' file
> set m 6
> get m
6
> add assets
> add src/fs.c
> list   
assets 
src/fs.c 
> analyze
> history
[1]     eseguito Tue Jun  9 17:37:11 2020
        lista delle risorse:
                - assets
                - src/fs.c
> report 1
1) Resoconto generale
2) Caratteri stampabili
3) Lettere
4) Spazi, numeri e punteggiatura
5) all
Inserire un numero per indicare una scelta: 5

File List:
assets/gen/lorem2.txt
src/fs.c
assets/gen/deep/very_deep/lorem3.txt
assets/bohemian_rhapsody.txt
assets/art/ackbar.txt
assets/genesis.txt
assets/art/dark_helmet.txt
assets/art/hypnotoad.txt
assets/info1.txt
assets/art/meow.txt
assets/info2.txt
assets/info3.txt
assets/info4.txt
assets/rand1.txt
assets/rand2.txt
assets/sourcecode/pthread.h
assets/sourcecode/stdlib.h
assets/gen/lorem1.txt

Caratteri totali rilevati: 396855

 Dati di ogni carattere ASCII:
caratteri 0x09 (HT): 1058 (0.27%)
caratteri 0x0A (LF): 11367 (2.86%)
[...]
caratteri 0x7E ('~'): 1 (0.00%)
```

### Parametri (opzionali)
- `-m`: la stringa seguente identifica il numero di slice in cui il file e' suddiviso
- `-n`: la stringa seguente identifica il numero di partitioni in cui dividere la lista dei file

# Eventuali problematiche e soluzioni di situazioni anomale

Abbiamo cercato di gestire le principali problematiche che possono verificarsi durante l'esecuzione del sistema:

- abbiamo gestito la correttezza dei parametri passati ai 3 programmi principali, `shell`, `analyzer` e `report`: ad esempio, `-m` essere seguito da una stringa valida, ovvero un numero intero positivo. I programmi "di appoggio", `reader`, `slicer` e `partitioner`, non fanno tali controlli
- abbiamo gestito la situazione in cui vengono "sporcati" i dati, o comunque sono passate stringhe *non* nel formato `file:carattere:occorrenze`, che vengono ignorate e non fanno crashare i programmi coinvolti
- abbiamo cercato di limitare i problemi che possono insorgere con le chiamate a funzioni e a system call, come `fork`, `exec`, `mkfifo`, ...
- ci risulta che tutte le risorse allocate nella heap vengano liberate non appena non sono piu' necessarie
- il comando `export` e' limitato alla directory corrente
- il passaggio a `report` di un file non formattato secondo lo standard `nomefile:carattere:occorrenze` non causa errori o interruzioni del programma, saranno semplicemente riportate a video 0 occorrenze. Allo stesso modo vengono ignorati tutti gli input che non rispettano tale formato.