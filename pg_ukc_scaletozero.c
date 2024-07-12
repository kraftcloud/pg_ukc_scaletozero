/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "executor/executor.h"

#include <unistd.h>
#include <fcntl.h>

PG_MODULE_MAGIC;

static ExecutorStart_hook_type onExecutorStartPrev = NULL;
static void onExecutorStart(QueryDesc *queryDesc, int eflags);

static ExecutorEnd_hook_type onExecutorEndPrev = NULL;
static void onExecutorEnd(QueryDesc *queryDesc);

static int disable_stz_fd = -1;

void _PG_init(void);
void _PG_init(void) 
{
	int err;
	int fd;
	
	fd = open("/uk/libukp/scale_to_zero_disable", O_WRONLY);
	if (fd == -1)
	{
		err = errno;
		ereport(WARNING, errmsg("Unable to open scale-to-zero control file: %d\n", err));
		return;
	}
	disable_stz_fd = fd;

	onExecutorStartPrev = ExecutorStart_hook;
	ExecutorStart_hook = onExecutorStart;

	onExecutorEndPrev = ExecutorEnd_hook;
	ExecutorEnd_hook = onExecutorEnd;
}

void _PG_fini(void);
void _PG_fini(void)
{
	if (disable_stz_fd == -1)
		return;

	ExecutorStart_hook = onExecutorStartPrev;
	ExecutorEnd_hook = onExecutorEndPrev;

	disable_stz_fd = -1;
}

static void onExecutorStart(QueryDesc *queryDesc, int eflags)
{
	ssize_t res;
	int err;

	if (disable_stz_fd != -1)
	{
		res = write(disable_stz_fd, "+", 1);
		if (res == -1) 
		{
			err = errno;
			ereport(WARNING, errmsg("Unable to write into scale-to-zero control file: %d\n", err));
		}
	}

	if (onExecutorStartPrev) {
		(*onExecutorStartPrev)(queryDesc, eflags);
	} else {
		standard_ExecutorStart(queryDesc, eflags);
	}
}

static void onExecutorEnd(QueryDesc *queryDesc)
{
	ssize_t res;
	int err;

	if (disable_stz_fd != -1)
	{
		res = write(disable_stz_fd, "-", 1);
		if (res == -1)
		{
			err = errno;
			ereport(WARNING, errmsg("Unable to write into scale-to-zero control file: %d\n", err));
		}
	}

	if (onExecutorEndPrev)
		(*onExecutorEndPrev)(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}
