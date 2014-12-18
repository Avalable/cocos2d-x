#include "JsonAllocator.h"
#include "ccMacros.h"
#include <stdlib.h>
#include <iostream>

#define JSON_ZONE_SIZE 4096
#define JSON_STACK_SIZE 32

void *JsonAllocator::allocate(unsigned size) {
	size = (size + 7) & ~7;

	if (head && head->used + size <= JSON_ZONE_SIZE) {
		char *p = (char *)head + head->used;
		head->used += size;
		return p;
	}

	size_t allocSize = sizeof(Zone) + size;
	Zone *zone = (Zone *)malloc(allocSize <= JSON_ZONE_SIZE ? JSON_ZONE_SIZE : allocSize);
	counter++;
	zone->used = allocSize;
	if (allocSize <= JSON_ZONE_SIZE || head == NULL) {
		zone->next = head;
		head = zone;
	}
	else {
		zone->next = head->next;
		head->next = zone;
	}
	return (char *)zone + sizeof(Zone);
}

void JsonAllocator::deallocate() {
	while (head) {
		Zone *next = head->next;
		free(head);
		head = next;
	}

//	std::cout << "Allocations: " << counter << std::endl;
}
