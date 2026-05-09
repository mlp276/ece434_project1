#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

#define NCHILD 4

static const int sigs[] = {
    SIGINT, SIGQUIT, SIGTSTP,
    SIGABRT, SIGILL, SIGCHLD,
    SIGSEGV, SIGFPE, SIGHUP
};
static const char *signames[] = {
    "SIGINT", "SIGQUIT", "SIGTSTP",
    "SIGABRT", "SIGILL", "SIGCHLD",
    "SIGSEGV", "SIGFPE", "SIGHUP"
};
#define NSIGS ((int)(sizeof(sigs) / sizeof(sigs[0])))

static const int first_half_block[]  = { SIGINT, SIGQUIT, SIGTSTP };
static const int first_half_n        = 3;
static const int second_half_block[] = { SIGABRT, SIGILL, SIGCHLD,
                                         SIGSEGV, SIGFPE, SIGHUP };
static const int second_half_n       = 6;

static volatile sig_atomic_t child_no = -1;

static const char *signame(int s)
{
    for (int i = 0; i < NSIGS; i++) if (sigs[i] == s) return signames[i];
    return "?";
}

static void die(const char *m) { perror(m); exit(1); }

static void handler(int signo, siginfo_t *info, void *uctx)
{
    (void)uctx;
    pid_t sender = info ? info->si_pid : 0;

    printf("Handler: pid=%ld child_no=%d signal=%d (%s) sender=%ld\n",
           (long)getpid(), child_no, signo, signame(signo), (long)sender);
    fflush(stdout);
}

static void install_handler_for_all(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    for (int i = 0; i < NSIGS; i++) sigaddset(&mask, sigs[i]);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_mask = mask;

    for (int i = 0; i < NSIGS; i++) {
        if (sigaction(sigs[i], &sa, NULL) < 0) die("sigaction");
    }
}

static void block_set(const int *list, int n)
{
    sigset_t s;
    sigemptyset(&s);
    for (int i = 0; i < n; i++) sigaddset(&s, list[i]);
    if (sigprocmask(SIG_BLOCK, &s, NULL) < 0) die("sigprocmask BLOCK");
}

static void unblock_set(const int *list, int n)
{
    sigset_t s;
    sigemptyset(&s);
    for (int i = 0; i < n; i++) sigaddset(&s, list[i]);
    if (sigprocmask(SIG_UNBLOCK, &s, NULL) < 0) die("sigprocmask UNBLOCK");
}

static void dump_pending(const char *who)
{
    sigset_t pend;
    if (sigpending(&pend) < 0) die("sigpending");
    printf("[%s pid=%ld] sigpending() ->", who, (long)getpid());
    int any = 0;
    for (int i = 0; i < NSIGS; i++) {
        if (sigismember(&pend, sigs[i])) {
            printf(" %s", signames[i]);
            any = 1;
        }
    }
    if (!any) printf(" <empty>");
    printf("\n");
    fflush(stdout);
}

static void dump_mask(const char *who)
{
    sigset_t cur;
    if (sigprocmask(SIG_BLOCK, NULL, &cur) < 0) die("sigprocmask GET");
    printf("[%s pid=%ld] current blocked mask ->", who, (long)getpid());
    int any = 0;
    for (int i = 0; i < NSIGS; i++) {
        if (sigismember(&cur, sigs[i])) {
            printf(" %s", signames[i]);
            any = 1;
        }
    }
    if (!any) printf(" <none>");
    printf("\n");
    fflush(stdout);
}

static void drain_nonblocking(const char *who, const int *list, int n)
{
    sigset_t s;
    sigemptyset(&s);
    for (int i = 0; i < n; i++) sigaddset(&s, list[i]);
    struct timespec ts = {0, 0};
    siginfo_t info;
    for (;;) {
        int got = sigtimedwait(&s, &info, &ts);
        if (got < 0) {
            if (errno == EAGAIN) {
                printf("[%s pid=%ld] sigtimedwait: EAGAIN (none pending)\n",
                       who, (long)getpid());
                fflush(stdout);
                return;
            }
            if (errno == EINTR) continue;
            die("sigtimedwait");
        }
        printf("[%s pid=%ld] sigtimedwait dequeued signal=%d (%s) "
               "sender=%ld\n", who, (long)getpid(), got, signame(got),
               (long)info.si_pid);
        fflush(stdout);
    }
}

static void drain_blocking(const char *who, const int *list, int n,
                           int count)
{
    sigset_t s;
    sigemptyset(&s);
    for (int i = 0; i < n; i++) sigaddset(&s, list[i]);
    siginfo_t info;
    for (int i = 0; i < count; i++) {
        int got = sigwaitinfo(&s, &info);
        if (got < 0) { if (errno == EINTR) { i--; continue; } die("sigwaitinfo"); }
        printf("[%s pid=%ld] sigwaitinfo dequeued signal=%d (%s) "
               "sender=%ld\n", who, (long)getpid(), got, signame(got),
               (long)info.si_pid);
        fflush(stdout);
    }
}

/*
 One full child run.
 */
static void child_main(int i)
{
    child_no = i;

    // Apply the half-specific block.
    if (i < NCHILD / 2) {
        printf("[child %d pid=%ld] first half - blocking "
               "{SIGINT,SIGQUIT,SIGTSTP}\n", i, (long)getpid());
        fflush(stdout);

        block_set(first_half_block, first_half_n);
    } else {
        printf("[child %d pid=%ld] second half - unblocking parent's "
               "{SIGINT,SIGQUIT,SIGTSTP} and blocking "
               "{SIGABRT,SIGILL,SIGCHLD,SIGSEGV,SIGFPE,SIGHUP}\n",
               i, (long)getpid());
        fflush(stdout);
        /* Drop the inherited block first. */
        unblock_set(first_half_block, first_half_n);
        block_set(second_half_block, second_half_n);
    }

    dump_mask("child");

    //sleep(30); //used for testing

    dump_pending("child");

    if (i < NCHILD / 2) drain_nonblocking("child", first_half_block, first_half_n);
    else drain_nonblocking("child", second_half_block, second_half_n);

    dump_pending("child");

    long long limit = 10LL * (long long)getpid();
    unsigned long long sum = 0;
    printf("[child %d pid=%ld] computing 0..%lld with sleep(10) per "
           "iteration\n", i, (long)getpid(), limit);
    fflush(stdout);

    long long bound = (limit < 5) ? limit : 5;
    for (long long k = 0; k <= bound; k++) {
        sum += k;
        unsigned int left = 10;
        while ((left = sleep(left)) > 0) { /* restart on EINTR */ }
    }
    printf("[child %d pid=%ld] done. partial-sum(0..%lld) = %llu "
           "(true sum to %lld omitted for runtime reasons)\n",
           i, (long)getpid(), bound, sum, limit);
    fflush(stdout);
    exit(0);
}

int main(void)
{
    printf("Parent pid=%ld - installing handlers for 9 signals\n",
           (long)getpid());
    install_handler_for_all();

    printf("Parent pid=%ld - blocking {SIGINT,SIGQUIT,SIGTSTP} before "
           "the pre-fork barrage\n", (long)getpid());
    block_set(first_half_block, first_half_n);
    dump_mask("parent");

    //sleep(30); //used for testing

    dump_pending("parent (pre-fork, after barrage)");

    drain_blocking("parent (pre-fork)", first_half_block, first_half_n, 3);
    dump_pending("parent (pre-fork, after sigwait drain)");

    drain_nonblocking("parent (pre-fork)", first_half_block, first_half_n);

    pid_t pids[NCHILD];
    for (int i = 0; i < NCHILD; i++) {
        pid_t p = fork();
        if (p < 0) die("fork");
        if (p == 0) {
            child_main(i);
            /* unreachable */
        }
        pids[i] = p;
        printf("Parent forked child %d -> pid %ld\n", i, (long)p);
        fflush(stdout);
    }

    //sleep(30); //used for testing

    dump_pending("parent (post-fork, after barrage)");
    drain_nonblocking("parent (post-fork)", first_half_block, first_half_n);
    dump_pending("parent (post-fork, after drain)");

    while (1) {
        int status;
        pid_t done = wait(&status);
        if (done > 0) {
            printf("Parent reaped child pid=%ld status=%d\n",
                   (long)done, status);
            fflush(stdout);
        } else if (done < 0 && errno == EINTR) {
            continue;
        } else if (done < 0 && errno == ECHILD) {
            break;
        } else {
            die("wait");
        }
    }

    printf("Parent restoring SIG_DFL for all 9 signals, then sleep(20)\n");
    fflush(stdout);

    unblock_set(first_half_block, first_half_n);
    for (int i = 0; i < NSIGS; i++) signal(sigs[i], SIG_DFL);
    sleep(20);
    printf("Parent finished\n");
    return 0;
}
