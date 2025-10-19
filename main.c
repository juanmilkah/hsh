#include <shell.h>
#include <utils.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

/**
 * readContents - reads the contents of a file descriptor
 * @fd: file descriptor
 * Return: pointer to the contents or NULL on failure
 */
static char *readContents(int fd)
{
	char tmp[1024];
	char *contents;
	ssize_t size, n = 0;

	contents = NULL;
	while ((size = read(fd, tmp, sizeof(tmp))) > 0) {
		n += size;
		contents = realloc(contents, n + 1);
		if (contents == NULL) {
			fdprint(STDERR_FILENO, "Error: malloc failed\n");
			return NULL;
		}
		strncpy(contents + n - size, tmp, size);
	}
	contents[n] = '\0';
	if (size < 0) {
		fdprint(STDERR_FILENO, "Error: Can't read file\n");
		return NULL;
	}
	return contents;
}
/**
 * runFile - runs the shell with the contents of a file descriptor
 * @fd: file descriptor
 * Return: exit status
 */
static int runFile(int fd)
{
	char *contents = readContents(fd);
	if (!contents) {
		close(fd);
		exit(127);
	}
	return loop(contents);
}

int main(int argc, char **argv)
{
	if (argc > 2) {
		fdprint(STDERR_FILENO, "Usage: %s [filename]", argv[0]);
		return (127);
	} else if (argc == 2) {
		int fd = open(argv[1], O_RDONLY);
		if (fd < 0) {
			fdprint(STDERR_FILENO, "Error: open failed\n");
			exit(127);
		}
		return (runFile(fd));
	} else if (!isatty(STDIN_FILENO)) {
		return (runFile(STDIN_FILENO));
	} else {
		return (loop(NULL));
	}
	return (0);
}
