#include <utils.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

void freevec(char **vector)
{
	char **tmp = vector;

	if (vector == NULL)
		return;
	while (*vector) {
		free(*vector);
		vector++;
	}
	free(tmp);
}

static void flush(int fd, char *buffer, int *len)
{
	if (*len >= BUFFER_SIZE) {
		write(fd, buffer, *len);
		*len = 0;
	}
}

void fdprint(int fd, const char *format, ...)
{
	va_list args;
	char buffer[BUFFER_SIZE];
	int len = 0;
	bool time_to_format = false;

	va_start(args, format);
	while (*format) {
		if (time_to_format) {
			char *s = va_arg(args, char *);
			for (int i = 0; s[i]; i++) {
				flush(fd, buffer, &len);
				buffer[len++] = s[i];
			}
			time_to_format = false;
			continue;
		} else if (*format == '%') {
			time_to_format = true;
		}
		buffer[len++] = *format;
		format++;
	}
}
