#pragma once
#include <stdlib.h>

class JsonAllocator {
	struct Zone {
		Zone *next;
		unsigned used;
	} *head = NULL;
	int counter;

public:

	JsonAllocator() : counter(0){}
	JsonAllocator(const JsonAllocator &);
	JsonAllocator &operator=(const JsonAllocator &);
	JsonAllocator(JsonAllocator &&x) : head(x.head) {
		x.head = NULL;
	}
	JsonAllocator &operator=(JsonAllocator &&x) {
		head = x.head;
		x.head = NULL;
		return *this;
	}

	~JsonAllocator() {
		deallocate();
	}

	void *allocate(unsigned size);
	void deallocate();
};
