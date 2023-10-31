/**
 * THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Yiwei Zhu
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "util.h"

/* '{' is next char to 'z', thus biggest than any word */
#define WORD_SENTINEL_CHAR '{'
#define WORD_SENTINEL_STR "{"

static int
parse(FILE *outs[], size_t len, int lng)
{
	size_t i;
	bool alleof;
	for (i = 0; ; )	// round robin on sorter i
	{
		size_t cnt = 0;
		int ch;
		if (i == 0)
		{
			alleof = TRUE;
		}
		while (feof(stdin) == 0 && (ch=fgetc(stdin)) != EOF)
		{
			alleof = FALSE;
			if (isalpha(ch))
			{
				if (++cnt > lng)
				{
					while (feof(stdin) == 0 && (ch=fgetc(stdin)) != EOF && isalpha(ch))
						;
					if (feof(stdin) == 0 && ch == EOF)
					{
						perror("fgetc");
						return 1;
					}
					if (feof(stdin))
						break;
					/* truncated, `ch` is first nonalpha char */
					ch = '\n';
					cnt = 0;
				} else
				{
					ch = tolower(ch);
				}
			} else
			{
				ch = '\n';
				cnt = 0;
			}
			if (fputc(ch, outs[i]) == EOF)
			{
				perror("fputc");
				return 1;
			}
			if (ch == '\n')
				break;
		}
		if (feof(stdin) == 0 && ch == EOF)
		{
			perror("fgetc");
			return 1;
		}
		/* round robin */
		if (++i == len)
		{
			i = 0;
			if (alleof)
				break;
		}
	}
	return 0;
}


/**
 * merge n word buffers of length `l` to another candidate word buffer of length `l`
 */
static int
merge(FILE *ins[], size_t len, int lng, int shrt)
{
	size_t i;
	const size_t buflen = lng + 1 + 1;	// `lng` chars + '\n' + '\0'
	char *buf = calloc(buflen, sizeof (char));
	char **sorter_bufs = calloc(len, sizeof (char*));
	for (i = 0; i < len; i++)
	{
		sorter_bufs[i] = calloc(buflen, sizeof (char));
	}
	bool *sbuf_eof = calloc(len, sizeof (bool));
	size_t neof = 0;
	int min_i;
	size_t freq = 0;
	while (neof != len)
	{
		min_i = -1;
		/* get min_i */
		size_t i;
		for (i = 0; i < len; i++) {
			if (sbuf_eof[i]) continue;
			if (min_i == -1 || strcmp(sorter_bufs[i], sorter_bufs[min_i]) < 0)
			{
				min_i = i;
			}
		}
		/* flush `buf` */
		if (strcmp(sorter_bufs[min_i], buf) == 0)
		{
			freq++;
		} else
		{
			if (buf[0] != '\0')
			{
				printf("%-10lu%s", freq, buf);
			}
			strcpy(buf, sorter_bufs[min_i]);
			freq = 1;
		}
		/* plump sorter min_i */
		if (fgets(sorter_bufs[min_i], buflen, ins[min_i]) == NULL)
		{
			neof++;
			sbuf_eof[min_i] = TRUE;
		} else
		{
			while (strlen(sorter_bufs[min_i])-1 <= shrt)
			{
				if (fgets(sorter_bufs[min_i], buflen, ins[min_i]) == NULL)
				{
					neof++;
					sbuf_eof[min_i] = TRUE;
					break;
				}
			}
		}
	}
	/* flush `buf` leftover */
	printf("%-10lu%s", freq, buf);
	
	for (i = 0; i < len; i++)
	{
		free(sorter_bufs[i]);
	}
	free(sbuf_eof);
	free(sorter_bufs);
	free(buf);
	return 0;
}


int
main(int argc, char *argv[])
{
	int n = 1;
	int s = -1, l = -1;
	int opt;
	size_t i;
	int waitstatus;
	const char *optstr = "n:s:l:";
	while ((opt=getopt(argc, argv, optstr)) != -1)
	{
		switch (opt)
		{
		case 'n':
			n = atoi(optarg);
			if (n <= 0)
			{
				fprintf(stderr, "expected a positive integer for option -n\n");
				return 1;
			}
			break;
		case 's':
			s = atoi(optarg);
			if (s < 0 || s == 0 && strcmp(optarg, "0") != 0)
			{
				fprintf(stderr, "expected a nonnegative integer for option -s\n");
				return 1;
			}
			break;
		case 'l':
			l = atoi(optarg);
			if (l <= 0)
			{
				fprintf(stderr, "expected a positive integer for option -l\n");
				return 1;
			}
			break;
		default:
			fprintf(stderr, "invalid option %c\n", opt);
			return 1;
		}
	}
	if (optind < argc)
	{
		fprintf(stderr, "redundant input\n");
		return 1;
	}
	if (s >= l)
	{
		fprintf(stderr, "s should be less than l\n");
		return 1;
	}

	int *pipefds = calloc(4*n, sizeof (int));
	for (i = 0; i < n; i++)	// for each sorter
	{
		/* pipefds[4*i..4*i+1] connects to parser, pipefds[4*i+2..4*i+3] connects to merger */
		if (pipe(pipefds + 4*i) != 0)
		{
			perror("pipe");
			return 1;
		}
		if (pipe(pipefds + 4*i+2) != 0)
		{
			perror("pipe");
			return 1;
		}
		if (fork() == 0)
		{
			/* sorter code */
			/* prep parser pipefd */
			if (close(pipefds[4*i+1]) != 0)	// close parser write side
			{
				perror("close");
				return 1;
			}
			if (dup2(pipefds[4*i], 0) < 0)	// dup parser read side to sorter's stdin
			{
				perror("dup2");
				return 1;
			}
			if (close(pipefds[4*i]) != 0)
			{
				perror("close");
				return 1;
			}
			/* prep merger pipefd */
			if (close(pipefds[4*i+2]) != 0)	// close merger read side
			{
				perror("close");
				return 1;
			}
			if (dup2(pipefds[4*i+3], 1) < 0)	// dup merger write side to sorter's stdout
			{
				perror("dup2");
				return 1;
			}
			if (close(pipefds[4*i+3]) != 0)
			{
				perror("close");
				return 1;
			}
			/* need to close leftover parent's pipefds[4*j + 2] and 4*j + 1, where 0 <= j < i */
			size_t j;
			for (j = 0; j < i; j++)
			{
				if (close(pipefds[4*j + 2]) != 0)
				{
					perror("close");
					return 1;
				}
				if (close(pipefds[4*j + 1]) != 0)
				{
					perror("close");
					return 1;
				}
			}
			execl("/usr/bin/sort", (char*)NULL);
			perror("execl");
			return 1;
		}
		/* parent close pipefds[4*i, 4*i+3]
		 * do not close 4*i + 2 since its later child merger will inherit this
		 */
		if (close(pipefds[4*i]) != 0)
		{
			perror("close");
			return 1;
		}
		if (close(pipefds[4*i + 3]) != 0)
		{
			perror("close");
			return 1;
		}
	}	// spawned all sorters
	/* parent only has pipefds[4*i + 1, 4*i + 2] open per i */
	/* give birth to merger process */
	if (fork() == 0)
	{
		/* merger */
		for (i = 0; i < n; i++)
		{
			if (close(pipefds[4*i + 1]) != 0)
			{
				perror("close");
				return 1;
			}
		}
		FILE **ins = calloc(n, sizeof (FILE*));
		for (i = 0; i < n; i++)
		{
			if ((ins[i]=fdopen(pipefds[4*i + 2], "r")) == NULL )
			{
				perror("fdopen");
				return 1;
			}
		}
		if (merge(ins, n, l, s) != 0)
		{
			return 1;
		}
		/* gc `ins` */
		for (i = 0; i < n; i++) {
			if (fclose(ins[i]) != 0)
			{
				perror("fclose");
				return 1;
			}
		}
		free(ins);
		return 0;
	}
	/* can close pipefds[4*i + 2] now that merger has been spawned */
	for (i = 0; i < n; i++)
	{
		if (close(pipefds[4*i + 2]) != 0)
		{
			perror("close");
			return 1;
		}
	}
	FILE **outs = calloc(n, sizeof (FILE*));
	for (i = 0; i < n; i++)
	{
		if ((outs[i]=fdopen(pipefds[4*i + 1], "w")) == NULL)
		{
			perror("fdopen");
			return 1;
		}
	}
	parse(outs, n, l);
	/* gc `outs` */
	for (i = 0; i < n; i++)
	{
		if (fclose(outs[i]) != 0)
		{
			perror("fclose");
			return 1;
		}
	}
	free(outs);
	/* wait for all sorter and merger */
	for (i = 0; i < n + 1; i++)
	{
		if (wait(&waitstatus) == -1)
		{
			perror("wait");
			return 1;
		}
		if (WIFEXITED(waitstatus) && WEXITSTATUS(waitstatus) != 0)
		{
			return WEXITSTATUS(waitstatus);
		}
	}
	free(pipefds);
	return 0;
}
