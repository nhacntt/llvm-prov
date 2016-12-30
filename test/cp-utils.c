/**
 * @file   cp-utils.c
 * @brief  integration test: a distillation of some FreeBSD cp(1) code
 *
 * RUN: %clang %cflags -S %s -D SOURCE="\"%s\"" -D DEST="\"%t.c\"" -emit-llvm -o %t.ll
 * RUN: %prov -S %t.ll -o %t.prov.ll
 * RUN: %filecheck %s -input-file %t.ll -check-prefix LLCHECK
 * RUN: %filecheck %s -input-file %t.prov.ll -check-prefix PROVCHECK
 */
/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#if 0
static char sccsid[] = "@(#)utils.c	8.3 (Berkeley) 4/1/94";
#endif
#endif /* not lint */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/acl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

int
copy_file(/*const FTSENT *entp, int dne*/)
{
	struct stat s;
	struct stat *fs = &s;
	ssize_t wcount;
	size_t wresid;
	off_t wtotal;
	int from_fd, rval, to_fd;
	char *bufp;
	char *p;

	from_fd = to_fd = -1;
	if ((from_fd = open(SOURCE, O_RDONLY)) == -1) {
		warn("%s", SOURCE);
		return (1);
	}
	fstat(from_fd, fs);

	to_fd = open(DEST, O_WRONLY | O_TRUNC | O_CREAT,
		fs->st_mode & ~(S_ISUID | S_ISGID));

	rval = 0;

	/* if (!lflag && !sflag) */ {
		/*
		 * Mmap and write if less than 8M (the limit is so we don't
		 * totally trash memory on big files.  This is really a minor
		 * hack, but it wins some CPU back.
		 * Some filesystems, such as smbnetfs, don't support mmap,
		 * so this is a best-effort attempt.
		 */
		// LLCHECK: call i8* @mmap
		// PROVCHECK: call i8* @metaio_mmap
		if (S_ISREG(fs->st_mode) && fs->st_size > 0 &&
		    fs->st_size <= 8 * 1024 * 1024 &&
		    (p = mmap(NULL, (size_t)fs->st_size, PROT_READ,
		    MAP_SHARED, from_fd, (off_t)0)) != MAP_FAILED) {
			wtotal = 0;
			for (bufp = p, wresid = fs->st_size; ;
			    bufp += wcount, wresid -= (size_t)wcount) {
				wcount = write(to_fd, bufp, wresid);
				if (wcount <= 0)
					break;
				wtotal += wcount;
				if (wcount >= (ssize_t)wresid)
					break;
			}
			if (wcount != (ssize_t)wresid) {
				warn("%s", DEST);
				rval = 1;
			}
			/* Some systems don't unmap on close(2). */
			if (munmap(p, fs->st_size) < 0) {
				warn("%s", SOURCE);
				rval = 1;
			}
		}
	}

	if (from_fd != -1)
		(void)close(from_fd);
	return (rval);
}