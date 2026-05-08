leaf() and nonleaf() should raise SIGTSTP right before exiting

At start of ~~non_leaf~~ fork_processes:
- Register signal handler for SIGUSR1, SIGINT using sigaction (gets registered for leaves as well)
- ~~Set disposition for SIGCHLD as SIG_IGN~~ No need since we use waitpid
- Need to set global exit_arg variable so that signal handlers can terminate with the correct unique process ID

How does child know which fate parent gave it?
- *Child doesn't need to know: it just receives signals, and signal handlers will take the appropriate action*

When does parent make decisions for its children?
- After receiving all children's data and before calling waitpid on children

Rule 1:
- Parent
    - Delivers SIGCONT
- Child
    - Sleeps 100
    - exit(return arg)

Rule 2:
- Parent
    - Delivers SIGCONT
    - Delivers SIGUSR1 with secret number using sigqueue()
- Child
    - As soon as it is continued, registers signal handler for SIGUSR1 using sigaction
    - Receives SIGUSR1 and receives secret number with siginfo_t
    - In the handler for SIGUSR1
        - Registers a second handler for signum secret number
        - Raises signal with signum secret number
    - Child enters handler for signum secret number
        - Terminates self with exit()

Rule 3:
- Parent
    - Delivers SIGCONT
    - Sleeps 10 seconds
    - Delivers SIGINT
- Child
    - Recives SIGINT
    - Handler for SIGINT: either registered to function that prints pid/ppid (experiment 1), or registerd to SIG_IGN (experiment 2)

After decision rule for each child, parent delivers SIGQUIT to all children

Parent updates its hidden node count to be smallest hidden count of all children