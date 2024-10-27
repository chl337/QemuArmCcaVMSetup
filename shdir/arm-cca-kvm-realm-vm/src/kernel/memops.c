/*
 * Copyright (c) 2013-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

void *memset(void *dst, int val, size_t count)
{
	uint8_t *ptr = dst;
	uint64_t *ptr64;
	uint64_t fill = (unsigned char)val;

	/* Simplify code below by making sure we write at least one byte. */
	if (count == 0U) {
		return dst;
	}

	/* Handle the first part, until the pointer becomes 64-bit aligned. */
	while (((uintptr_t)ptr & 7U) != 0U) {
		*ptr = (uint8_t)val;
		ptr++;
		if (--count == 0U) {
			return dst;
		}
	}

	/* Duplicate the fill byte to the rest of the 64-bit word. */
	fill |= fill << 8;
	fill |= fill << 16;
	fill |= fill << 32;

	/* Use 64-bit writes for as long as possible. */
	ptr64 = (uint64_t *)ptr;
	for (; count >= 8U; count -= 8) {
		*ptr64 = fill;
		ptr64++;
	}

	/* Handle the remaining part byte-per-byte. */
	ptr = (uint8_t *)ptr64;
	while (count-- > 0U)  {
		*ptr = (uint8_t)val;
		ptr++;
	}

	return dst;
}
int memcmp(const void *s1, const void *s2, size_t n)
{
	if (n != 0) {
		const unsigned char *p1 = s1, *p2 = s2;

		do {
			if (*p1++ != *p2++)
				return (*--p1 - *--p2);
		} while (--n != 0);
	}
	return (0);
}

void *memcpy(void *s1, const void *s2, size_t n)
{
	const char *f = s2;
	char *t = s1;

	while (n-- > 0)
		*t++ = *f++;
	return s1;
}
