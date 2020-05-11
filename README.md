# Organizzazione del codice in file
*Qui spieghiamo la struttura dell'archivio che gli inviamo: cosa c'e' in src/, cosa contiene ogni file sorgente, e robe varie*

# Architettura
*Qui spieghiamo a grandi linee come funziona ed il perche' delle nostre scelte. Poi verra' spiegato piu' in dettaglio nelle sottosezioni*

## Shell (aka M)
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

## Report (aka R)
Report esegue delle statitiche ricevuto in input.
Dovrebbe ricevere in qualche modo dati da un processo Analyzer separato.

Esempio:
```
>./report
pid: 69420
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

Esempio chiamata per la lettura di un salvataggio dell'output di Analysis:
```
./report < salvataggio.txt
```

## Analyzer (aka A)
Analyzer e' il programma di avvio delle analisi, avviando istanze di Partitioner sui parametri passati ad esso, ricevendone i risultati e stampandoli in stdout.
Dovrebbe inviare in qualche modo dati da un processo Report separato.

Esempio:
```
>./analyzer -n 3 -m 4 file1.txt file2.txt
file1.txt:65:27
file1.txt:66:393
file2.txt:65:42
```

Esempio chiamata per salvataggio dei risultati:
```
>./analyzer -n 3 -m 4 file1.txt file2.txt > salvataggio.txt
```

Esempio chiamata comunicandogli il pid di Report:
```
>./analyzer -r 69420 -n 3 -m 4 file1.txt file2.txt
```

## Partitioner (aka C)
Partitioner e' il programma che riceve i parametri, in particolare n e la lista di file, e crea un partizionamento di n partizioni contenenti ogniuno lo stesso numero di file (ad eccezione di una eventuale parizione che ne puo' avere una di meno). Stampa i risultati in stout.

Esempio:
```
>./partitioner -n 3 -m 4 file1.txt file2.txt
file1.txt:65:27
file1.txt:66:393
file2.txt:65:42
```

## Slicer (aka P)
Slicer e' il programma che esegue l'analisi sulla partizione di file ricevuta, parallelizzando la lettura in m processi Reader, ogniuno con la propria slice. Funziona standalone.

Il thread principale avvia un numero di processi Reader pari al numero di slice di cui i file sono composti. Questi funzionano esattamente come descritto nella sezione Reader, meno il fatto che il loro output e' scritto in una pipe specifica per il processo in questione. Inoltre, per ogni processo, viene avviato anche un thread di ascolto, che e' adibito alla lettura della pipe in cui un processo Reader scrive.

Una volta terminati i thread di ascolto, il programma stampa il resoconto delle occorrenze di caratteri per ogni file di tutte le sue slice, quindi del file intero.

Il sistema utilizza quindi ```fork()```, ```execve()```, ```pipe()```, ```dup2()``` e ```pthread```. Le prime quattro per avvia processi Reader ed instaurare una comunicazione tra essi e lo Slicer, l'ultima per leggere l'output prodotto dai processi Reader.

Esempio chiamata e di output da linea di comando:
```sh
>./slicer -m 4 file1.txt file2.txt  # lettura dei file file1.txt e file2.txt, ogniuno visto come composto di 4 slice
file1.txt:65:108    # file1.txt: il carattere ASCII 65 ('A') appare 108 volte (in tutte le sue slice)
file1.txt:66:1572   # file1.txt: il carattere ASCII 66 ('B') appare 1572 volte (in tutte le sue slice)
file2.txt:65:260    # file2.txt: il carattere ASCII 65 ('A') appare 260 volte (in tutte le sue slice)
file2.txt:66:16     # file2.txt: il carattere ASCII 66 ('B') appare 16 volte (in tutte le sue slice)
```

## Reader (aka Q)
Reader e' il programma che legge una specifica slice di ogni file indicatogli, parallelizzando la lettura tra diversi thread. Funziona standalone.

Il thread principale avvia un numero di thread di lettura pari al numero di file ricevuti alla chiamata. Ogniuno di questi legge uno dei file in questione nella sola slice che gli compete. Una volta letta, il thread di lettura termina.

Quando tutti i thread di lettura delle slice sono terminati, il thread principale stampa in output il resoconto delle occorrenze di carattere per ogni file nella slice richiesta. Occorrenze pari a 0 sono ignorate.

Il sistema utilizza esclusivamente ```pthread```, senza utilizzare tecniche si sincronizzazione, dato che ogni thread di lettura, pur condividendo lo stesso spazio di indirizzamento, accedere ad un'area di memoria a lui esclusiva durante la sua esecuzione.

Esempio chiamata e di output da linea di comando:
```sh
>./reader -s 2 -m 4 file1.txt file2.txt # lettura della slice 2 di 4 dei file file1.txt e file2.txt
file1.txt:65:27     # file1.txt: il carattere ASCII 65 ('A') appare 27 volte (nella slice 2/4)
file1.txt:66:393    # file1.txt: il carattere ASCII 66 ('B') appare 393 volte (nella slice 2/4)
file2.txt:65:65     # file2.txt: il carattere ASCII 65 ('A') appare 65 volte (nella slice 2/4)
file2.txt:66:4      # file2.txt: il carattere ASCII 66 ('B') appare 4 volte (nella slice 2/4)
```