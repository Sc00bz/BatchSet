#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "batchset.h"

int main()
{
	BatchSet<uint64_t>  mySet;
	uint64_t           *first  = new uint64_t[64 * 1024];
	uint64_t           *last   = first;
	int                 errors = 0;

	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 1024; j++)
		{
			uint64_t x = (uint64_t) rand();

			*last = x;
			last++;
			mySet.insert(x);
		}

		// Merge
		std::sort(first, last);
		last = std::unique(first, last);
		if (mySet.count() != last - first)
		{
			errors++;
		}
		else
		{
			for (int j = 0; j < last - first; j++)
			{
				if (mySet[j] != first[j])
				{
					errors++;
				}
			}
		}
	}
	if (errors != 0)
	{
		printf("%u errors\n", errors);
	}
	else
	{
		printf("Success\n");
	}
	return 0;
}
