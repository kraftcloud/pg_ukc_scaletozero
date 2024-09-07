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

static ExecutorRun_hook_type onExecutorRunPrev;
static void onExecutorRun(QueryDesc *queryDesc,
						  ScanDirection direction,
						  uint64 count,
						  bool execute_once);

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

	onExecutorRunPrev = ExecutorRun_hook;
	ExecutorRun_hook = onExecutorRun;
}

void _PG_fini(void);
void _PG_fini(void)
{
	if (disable_stz_fd == -1)
		return;

	ExecutorRun_hook = onExecutorRunPrev;

	close(disable_stz_fd);
	disable_stz_fd = -1;
}

static void ctrlScaleToZero(int disable)
{
	ssize_t res;
	int err;

	if (disable_stz_fd == -1)
		return;

	res = write(disable_stz_fd, (disable) ? "+" : "-", 1);
	if (res == -1)
	{
		err = errno;
		ereport(WARNING, errmsg("Unable to write into scale-to-zero control file: %d\n", err));
	}
}

static void onExecutorRun(QueryDesc *queryDesc,
						  ScanDirection direction,
						  uint64 count,
						  bool execute_once)
{

	ctrlScaleToZero(1);

	PG_TRY();
	{
		if (onExecutorRunPrev) {
			(*onExecutorRunPrev)(queryDesc, direction, count,
								 execute_once);
		} else {
			standard_ExecutorRun(queryDesc, direction, count,
								 execute_once);
		}
	}
	PG_FINALLY();
	{
		ctrlScaleToZero(0);
	}
	PG_END_TRY();
}
