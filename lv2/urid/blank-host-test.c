/*
  Copyright 2019 David Robillard <http://drobilla.net>
  Copyright 2019 Hanspeter Portner <https://open-music-kontrollers.ch>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <string.h>
#include <stdlib.h>

#ifndef __STDC_NO_THREADS__
#	include <threads.h>
#else
#	include <pthread.h>
#endif

#include "lv2/core/lv2.h"
#include "lv2/urid/blank-host.h"

#define MAX_BLANK   0x1000000
#define MAX_THREADS 12
#define MAX_IDS     (MAX_BLANK*MAX_THREADS)

static LV2_URID_Blank lv2_urid_blank = {
	.handle = NULL,
	.id = lv2_urid_blank_id
};

static const LV2_Feature lv2_urid_blank_feature = {
	.URI = LV2_URID__blank,
	.data = &lv2_urid_blank
};

static atomic_flag flags [MAX_IDS];

static void *
run(void *data)
{
	LV2_URID_Blank *blank = data;

	int *res = calloc(1, sizeof(int));
	if(!res)
	{
		return NULL;
	}
	
	for(LV2_URID_Blank_ID i = 0, last = 0; i < MAX_BLANK; i++)
	{
		const LV2_URID_Blank_ID id = blank->id(blank->handle, 0);

		if(atomic_flag_test_and_set_explicit(&flags[id], memory_order_relaxed))
		{
			*res |= 1;
		}

		last = id;
	}

	return res;
}

#ifndef __STDC_NO_THREADS__
static int
start(void *data)
{
	int *res = run(data);
	if(!res)
	{
		return 1;
	}

	free(res);
	return 0;
}
#endif

int
main(void)
{
	const LV2_Feature *feat = &lv2_urid_blank_feature;
	if(!feat)
	{
		goto fail;
	}

	if(strcmp(feat->URI, LV2_URID__blank) != 0)
	{
		goto fail;
	}

	LV2_URID_Blank *blank = feat->data;
	if(!blank)
	{
		goto fail;
	}

	for(int i = 0; i < MAX_IDS; i++)
	{
		atomic_flag_clear_explicit(&flags[i], memory_order_relaxed);
	}

#ifndef __STDC_NO_THREADS__
	thrd_t threads [MAX_THREADS];
#else
	pthread_t threads [MAX_THREADS];
#endif

	for(int i = 0; i < MAX_THREADS; i++)
	{
#ifndef __STDC_NO_THREADS__
		if(thrd_create(&threads[i], start, blank) != thrd_success)
#else
		if(pthread_create(&threads[i], 0, run, blank) != 0)
#endif
		{
			for(i--; i >= 0; i--)
			{
#ifndef __STDC_NO_THREADS__
				thrd_join(threads[i], NULL);
#else
				int *res = NULL;

				pthread_join(threads[i], &res);

				if(res)
				{
					free(res);
				}
#endif
			}

			goto fail;
		}
	}

	int ret = 0;

	for(int i = 0; i < MAX_THREADS; i++)
	{
#ifndef __STDC_NO_THREADS__
		int res = 0;

		if(thrd_join(threads[i], &res) != thrd_success)
		{
			ret |= 1;
			continue;
		}

		ret |= res;
#else
		int *res = NULL;

		if(pthread_join(threads[i], (void **)&res) != 0)
		{
			ret |= 1;
			continue;
		}

		if(!res)
		{
			ret |= 1;
			continue;
		}

		ret |= *res;
		free(res);
#endif
	}

	if(ret != 0)
	{
		goto fail;
	}

	const LV2_URID_Blank_ID id = blank->id(blank->handle, 0);
	if(id != MAX_IDS)
	{
		goto fail;
	}

	return 0;

fail:
	return 1;
}
