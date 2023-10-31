# cs551-system-programming-pipesort
cs551 System Programming coursework—implementing "parallel sort" via POSIX pipe.

The idea is to practice Inter Process Communication (IPC) via syscall `pipe`, `fork` and `execl` by writing a CLI program. There will be one parser process that takes `stdin` and writes to pipes connected with sorter processes newline-terminated words. Sorters run `/usr/bin/sort` and writes the output to pipes connected with a merger process, which then outputs to `stdout` frequency of words, sorted.

## files
See "pipe-sort.pdf" for what the coursework is about.

`pipesort-4-10-sh` shows pipelines using POSIX shell utilities to achieve similar results, as running `./pipesort -s 4 -l 10`. It is not part of the homework.

## build instructions
Type `make` in a shell for generating the executable "pipesort". Type `make clean` to get rid of it.
