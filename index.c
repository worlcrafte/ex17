#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

int num;

void int_handler(int sig)
{
    static int nsig = 0;
    // Le mot clef static permet de réduire la visibilité de l'élément au bloc dans lequel il est défini. Dans le cas d'une variable globale qualifiée de statique, la notion de bloc correspond au fichier d'implémentation dans lequel elle est définie. On ne peut donc pas l'utiliser de l'extérieur du fichier considéré.
    // Elle est donc limité à chacun des enfants. Et est def qu'une seul fois de plus peut être considérer aussi comme une variable globale.
    nsig++;
    if (!(nsig < num))
    {
        // printf("nsig: %i\n",nsig);
        exit(0);
    }
}

void attente()
{
    struct sigaction sa; // cette structure va nous permettre d'interpréter un signale et de réalisé une action en conséquence de celui ci.

    /* ça strucure :
            struct sigaction {
               void     (*sa_handler)(int);
               void     (*sa_sigaction)(int, siginfo_t *, void *);
               sigset_t   sa_mask;
               int        sa_flags;
               void     (*sa_restorer)(void);
           };
    */
    printf("nb: %i pid: %i \n", num, getpid());
    sa.sa_handler = int_handler;
    sa.sa_falgs = SA_RESTART;
    if (sigemptyset(&sa.sa_mask) == -1)
    {
        // sigemptyset() vide l'ensemble de signaux fourni par set, tous les signaux étant exclus de cet ensemble.
        // je n'exclu aucun signale.
        perror("sigemptyset");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        // Si act est non nul, la nouvelle action pour le signal signum est définie par act. Si oldact est non nul, l'ancienne action est sauvegardée dans oldact.

        // sa_handler indique l'action affectée au signal signum, et peut être SIG_DFL pour l'action par défaut, SIG_IGN pour ignorer le signal, ou un pointeur sur une fonction de gestion de signaux. Cette fonction reçoit le numéro de signal comme seul argument.

        // sa_mask fournit un masque de signaux à bloquer pendant l'exécution du gestionnaire. De plus, le signal ayant appelé le gestionnaire est bloqué à moins que l'attribut SA_NODEFER ne soit précisé.

        // sa_flags spécifie un ensemble d'attributs qui modifient le comportement du signal.

        perror("sigaction");
    }
    while (1)
    {
        pause();
    } // on attends de manière indéfinie de recevoir un signale qui sera interpréter par une foction gestionnaire dans sa_andler.
}

int main()
{
    int compteur = 0;
    pid_t child;
    pid_t grp = -1;
    int status;
    errno = 0;
    for (int i = 0; i < 10; i++)
    { // à l'aide de cette boucle on attends aucun enfant.On a donc belle est bien un processus //.
        // printf("%i",i);
        child = fork();
        if (child == -1)
        {
            perror("fork");
            // n'existe pas car on doit gérer d'autre enfant
        }
        else if (child == 0)
        {
            // dans l'enfant
            srand(getpid());
            num = rand() % 11; // pour que ça soit un chiffre aléatoire entre 0 et 10.
            // num=5;
            if (num == 0)
            {
                num++;
            }
            attente(); // on le place dans la fct attente
        }
        else
        {
            if (grp == -1)
            { // on prend en compte l'échec de l'exécution de fork.
                grp = child;
                // on mémorise qu'on veut créer un grp de processus. Celui portera l'id du premier enfant.
            }
            if (setpgid(child, grp) == -1)
            {
                // printf("eru");
                perror("setpgip");
            }
        }
    }
    // printf("ok");
    sleep(1); // attends que tous les enfants ce lance
    while (1)
    {
        if (kill(-grp, SIGTERM) == -1)
        {
            perror("kill");
            continue;
        }
        compteur++;
        sleep(1);                                // attends que tous les enfants recoivent le signal.
        child = waitpid(-grp, &status, WNOHANG); // WNOHANG permet de retourner une valeur même si aucun enfant n'est fini. Dans ce cas la child sera égale à 0.

        if (child == -1)
        {
            if (errno == ECHILD)
            { // il se peut qu'aucun enfant ne c créée. Mais aussi qu'on est tué tous les enfants.
                exit(EXIT_SUCCESS);
            }
            else
            {
                perror("wait"); // si ce n'est pas la situation du dessus alors je le traite comme une erreur.
                exit(EXIT_FAILURE);
            }
        }
        else if (WEXITSTATUS(status)) // on examine pk l'enfant à terminé.
        {
            // l'enfant à échoué
            printf("child %i exited with %i\n", child, status);
        }
        else if (child != 0)
        {
            // child success
            printf("nb trouvé: %i, pid de l'enfant: %i\n", compteur, child);
            if (kill(child, SIGKILL) == -1)
            {
                perror("kill");
                continue;
            }
            while ((child = waitpid(-grp, &status, WNOHANG)) != 0 && (child != -1))
            {
                printf("nb trouvé: %i, pid de l'enfant: %i\n", compteur, child);
                if (kill(child, SIGKILL) == -1)
                {
                    perror("kill");
                    continue;
                }
            }
        }
    }
    exit(0);
}
