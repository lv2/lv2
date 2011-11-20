/* LV2 OSC Messages Extension - Pretty printing methods
 * Copyright (C) 2007-2009 David Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "lv2_osc_print.h"

void
lv2_osc_argument_print(char type, const LV2_OSC_Argument* arg)
{
	int32_t blob_size;

    switch (type) {
	case 'c':
		printf("%c", arg->c); break;
	case 'i':
		printf("%d", arg->i); break;
	case 'f':
		printf("%f", arg->f); break;
	case 'h':
		printf("%ld", arg->h); break;
	case 'd':
		printf("%f", arg->d); break;
	case 's':
		printf("\"%s\"", &arg->s); break;
	/*case 'S':
		printf("\"%s\"", &arg->S); break;*/
	case 'b':
		blob_size = *((int32_t*)arg);
		printf("{ ");
		for (int32_t i=0; i < blob_size; ++i)
			printf("%X, ", (&arg->b)[i+4]);
		printf(" }");
		break;
	default:
		printf("?");
	}
}


void
lv2_osc_print(const LV2_OSC_Event* msg)
{
	const char* const types = lv2_osc_get_types(msg);

	printf("%s (%s) ", lv2_osc_get_path(msg), types);
	for (uint32_t i=0; i < msg->argument_count; ++i) {
		lv2_osc_argument_print(types[i], lv2_osc_get_argument(msg, i));
		printf(" ");
	}
	printf("\n");
}

