1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  If we call execvp() directly, the shell would get replaced and disappear. fork() makes a child run the command so the shell stays alive

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  fork() returns -1 (no child). My code prints an error with perror("fork") and returns an error code instead of crashing

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  execvp() searches folders listed in the PATH environment variable (like /bin, /usr/bin)

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  It lets the parent collect the child’s exit code and prevents zombie processes. Without it, finished children can pile up as zombies

   5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    >    **Answer**:  t extracts the child’s real exit code (like 0 for success) from the waitpid() status value

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  If it sees ' or ", it keeps everything inside as one argument (spaces included) and removes the quotes. Needed for stuff like echo "hello world"

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  Mainly I made sure argv[argc] = NULL every time so execvp() works. Biggest annoyance was handling spaces + quotes correctly while using one buffer

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  Signals are a quick way for the operating system or another process to get a process’s attention. It is more like a notification or command such as “stop” or “quit.” Other IPC like pipes or sockets are used to actually pass data back and forth, but signals are mostly for control, not for sending information

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  SIGINT is what happens when you press Ctrl C. It is used to interrupt a program and stop it
  SIGTERM is the normal way to ask a program to exit. Programs can usually clean up and close files before quitting
  SIGKILL is the forced shutdown. It stops the process immediately and the process does not get a chance to clean up

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  SIGSTOP pauses the process right away. The process cannot catch it and cannot ignore it. The reason is because SIGSTOP is meant to be a guaranteed stop that the kernel controls, like when you pause a job in the termina
