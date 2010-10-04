#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <unistd.h>
#include "lv2_contexts.h"

#define TEST_ASSERT(check) do {\
	if (!(check)) {\
		fprintf(stderr, "Failure at line %d: %s\n", __LINE__, #check);\
		assert(false);\
		_exit(1);\
	}\
} while (0)

#define NUM_PORTS 64

void
print_flags(void* flags)
{
	for (int i = NUM_PORTS; i >= 0; --i)
		printf((lv2_contexts_port_is_valid(flags, i)) ? "1" : "0");
	printf("\n");
}


int
main()
{
	uint64_t flags = 0;
	print_flags(&flags);

	lv2_contexts_set_port_valid(&flags, 16);
	print_flags(&flags);
	for (int i = 0; i < NUM_PORTS; ++i) {
		if (i == 16) {
			TEST_ASSERT(lv2_contexts_port_is_valid(&flags, i));
		} else {
			TEST_ASSERT(!lv2_contexts_port_is_valid(&flags, i));
		}
	}

	lv2_contexts_set_port_valid(&flags, 46);
	lv2_contexts_set_port_valid(&flags, 0);
	print_flags(&flags);
	for (int i = 0; i < NUM_PORTS; ++i) {
		if (i == 0 || i == 16 || i == 46) {
			TEST_ASSERT(lv2_contexts_port_is_valid(&flags, i));
		} else {
			TEST_ASSERT(!lv2_contexts_port_is_valid(&flags, i));
		}
	}

	lv2_contexts_unset_port_valid(&flags, 16);
	print_flags(&flags);
	for (int i = 0; i < NUM_PORTS; ++i) {
		if (i == 0 || i == 46) {
			TEST_ASSERT(lv2_contexts_port_is_valid(&flags, i));
		} else {
			TEST_ASSERT(!lv2_contexts_port_is_valid(&flags, i));
		}
	}

	return 0;
}

