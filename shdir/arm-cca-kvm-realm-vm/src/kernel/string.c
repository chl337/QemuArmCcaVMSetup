#include <string.h>
#include <stddef.h>

int strncmp(const char *s1, const char *s2, size_t n)
{

	if (n == 0)
		return (0);
	do {
		if (*s1 != *s2++)
			return (*(unsigned char *)s1 - *(unsigned char *)--s2);
		if (*s1++ == 0)
			break;
	} while (--n != 0);
	return (0);
}

int strcmp(const char *s1, const char *s2)
{
	for (; *s1 == *s2 && *s1; s1++, s2++);
	return (unsigned char)*s1 - (unsigned char)*s2;
}

size_t strlen(const char *buffer)
{
	const char *ptr = buffer;
	while (*ptr++)
		continue;
	return (ptr - buffer - 1);
}

char * strcpy(char *to, const char *from)
{
	while (*from)
		*to++ = *from++;
	*to++ = '\0';
	return (to);
}

char * strncpy(char *dst, const char *src, size_t n)
{
	if (n != 0) {
		char *d = dst;
		const char *s = src;

		do {
			if ((*d++ = *s++) == 0) {
				while (--n != 0)
					*d++ = 0;
				break;
			}
		} while (--n != 0);
	}
	return (dst);
}

char * strrchr(const char *s, int c)
{
	char *t = NULL;

	while (*s) {
		if (*s == c)
			t = (char *)s;
		s++;
	}
	return (t);
}
