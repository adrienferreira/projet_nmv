# projet_nmv
Invité de commande pour le noyau Linux. À rendre avant le samedi 16 janvier, 23h59 à maxime.lorrillere@lip6.fr

# Mode d'emploi
Notre invite de commande est de type commande/sous-comande à la manière de Git.
Avant d'executer le Makefile qui est à al racine du projet, il faut prendre soin de renseigner la variable d'environnement KERNELDOIR.
Par exemple en éditant le Makefile à la racine du projet qui se répercutera sur tous les Makefiles du projet.

Le dossier src contient 3 sous-répertoires :
	- shmodule : qui contient les source de l'unique module shmod.ko
	- shellghoumi : qui contient l'invite de commande pour envoyer les IOCTL sur le module
	- test ??

Une fois compilé, le programme s'exécute ainsi :
	- insmod src/shmodule/shmod.ko
	- mknode /dev/shmodule c numMajeur 0
	- se déplacer dans src/shellghoumi
	- tapper la commande souhaitée comme suit :
		./shellghoumi [-b] <command> [<args>]
		Par exemple :
			./shellghoumi -b kill 402 15

L'option -b est optionnelle mais doit obligatoirement figurer en premier argument du programme.
Le mode asynchrone se lance avec l'option -b.
Pour récupérer le résultat, toute commande lancée avec l'option -b affiche un numéro de ticket à renseigner pour récupérer le résultat ultérieurement.
Est aussi affiché la taille de la valeur de retour à passer à la commande de récupération.

Par exemple :
	- ./shellghoumi -b kill 402 15
	- L'invite affiche que notre numéro de ticket est 42 et que la taille de la valeur de retour est de 8
	- ./shellghoumi return 42 8
	- L'invite affiche la valeur de retour de la commande kill.

À noter que l'ioctl return est bloquante tant que le résultat demandé n'est pas prêt.

# Traces
kill synchrone:
[root@vm-nmv project]# sleep 90 &
[1] 319
[root@vm-nmv project]# ./shellghoumi kill 317 9
[  249.352380] No process with the given PID
[root@vm-nmv project]# ./shellghoumi kill 319 9
[1]+  Killed                  sleep 90

kill asynchrone:
[root@vm-nmv project]# sleep 180 &
[1] 326
[root@vm-nmv project]# ./shellghoumi -b kill 326 9
Async call got number 0 
Reclaim result with 'return 0 8' command
[root@vm-nmv project]# echo "coucou"
coucou
[1]+  Killed                  sleep 180
[root@vm-nmv project]# ./shellghoumi return 0 8
Return kill : 0

Wait synchrone:
[root@vm-nmv project]# sleep 33&
[1] 303
[root@vm-nmv project]# sleep 44&
[2] 304
[root@vm-nmv project]# ./shellghoumi wait 303 304
[  350.730010] Process finished 0/1
[  351.730011] Process finished 0/1
[  352.730010] Process finished 0/1
[  353.730013] Process finished 0/1
[  354.730018] Process finished 0/1
[  355.730014] Process finished 0/1
[  356.730009] Process finished 0/1
[  357.730011] Process finished 0/1
[  358.730009] Process finished 0/1
[  359.730009] Process finished 0/1
[  360.730010] Process finished 0/1
[  361.730010] Process finished 0/1
[  362.730010] Process finished 0/1
[  363.730011] Process finished 0/1
[  364.730013] Process finished 0/1
[  365.730010] Process finished 0/1
[  366.730013] Process finished 0/1
[  367.730018] Process finished 1/1
[1]-  Done                    sleep 33

wait asynchrone:
[root@vm-nmv project]# sleep 33&
[1] 307
[root@vm-nmv project]# ./shellghoumi -b wait 307
Async call got number 0 
Reclaim result with 'return 0 4' command
[root@vm-nmv project]#
[1]+  Done                    sleep 33
[root@vm-nmv project]# ./shellghoumi return 0 4
Return wait : 1

Waitall suit le même principe.

lsmod synchrone:
[root@vm-nmv project]# ./shellghoumi lsmod
Module			Size  Used by
shmod                   9035  0
weasel                  1744  0
helloWorld              1194  0

lsmod asynchrone:
[root@vm-nmv project]# ./shellghoumi -b lsmod
Async call got number 1 
Reclaim result with 'return 1 640' command
[root@vm-nmv project]# ./shellghoumi return 1 640
Module			Size  Used by
shmod                   9035  0
weasel                  1744  0
helloWorld              1194  0

print meminfo synchrone:
[root@vm-nmv project]# ./shellghoumi print meminfo
[ 1878.027566] meminfo: MemTotal: 2044996
MemTotal:      2044996 kB
MemFree:       1964696 kB
MemAvailable:        0 kB
Buffers:          9172 kB
SwapTotal:           0 kB
SwapFree:            0 kB
Shmem:            8620 kB

print meminfo asynchrone:
[root@vm-nmv project]# ./shellghoumi -b print meminfo
Async call got number 0 
Reclaim result with 'return 0 112' command
[root@vm-nmv project]# ./shellghoumi return 0 112
MemTotal:      2044996 kB
MemFree:       1964640 kB
MemAvailable:        0 kB
Buffers:          9176 kB
SwapTotal:           0 kB
SwapFree:            0 kB
Shmem:            8620 kB


# Explications
Asynchronisme
Quand une commande est lancée avec l'option -b, le traitement est placé dans la workqueue système.
Nous avons crée une liste chainée de résultats non-récupérés (struct pend_res), elle est protégée par un mutex global.
À chaque commande asynchrone, un maillon est créé et passé en paramètre au traitant de la tâche asynchrone.
Ce maillon possède un identifiant qui lui est assigné grâce à un compteur global.
Cet identifiant est remonté dans l'espace utilisateur et affiché dans l'invité pour que l'utilisateur puisse récupérer le résultat de sa commande.
Quand un traitant de commande dans la workqueue se termine, il place son résultat dans le maillon et réveille tous ceux qui attendaient la termianison du traitement.
L'IOCTL return utilise une waitqueue pour s'endormir sur un maillon dont il attent le remplissage par un thread worker.

wait et waitall sont dirigés vers le même traitant d'IOCTL.
Le wait n'étant qu'un cas particulier du waitall où le nombre de processus à attendre est égal toujours à 1, nous avons pu factoriser le code.

Le tread worker de wait et waitall ne fait que boucler sur tous les PIDs en vérifiant qu'ils sont bien "alive".
Si le nombre définit de PIDs à attendre n'est pas atteint, il se replace dans la delayed workqueue.
La fréquence de vérification des PIDs a arbitrairement été fixé à une seconde et peut être modifié grâce à un define.

#Reste à faire
Il n'est pas possible de récupérer simultanément le résultat d'une même commande asynchrone.
