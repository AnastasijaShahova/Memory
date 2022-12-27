#pragma once
#include <cassert>
#include <windows.h>

#include "FixedSizeAllocator.h"
#include "CoalesceAllocator.h"
#include <iostream>
#include <vector>


#define SIZE 10485760

class MemoryAllocator{
public:
	MemoryAllocator();
	virtual ~MemoryAllocator();

	virtual void init();
	virtual void destroy();

	virtual void *alloc(size_t size);
	virtual void free(void* p);

#ifdef _DEBUG
	virtual void dumpStat() const;
	virtual void dumpBlocks() const;
#endif

private:
	
#ifdef _DEBUG
	bool is_initialized;
	bool is_destroyed;

	size_t num_alloc;
	size_t num_free;
#endif  

	struct Block {
		size_t size;
		void* data;
	};

	std::vector<Block> OSBlocks;

	FixedSizeAllocator fsa16;
    FixedSizeAllocator fsa32;
    FixedSizeAllocator fsa64;
    FixedSizeAllocator fsa128;
    FixedSizeAllocator fsa256;
    FixedSizeAllocator fsa512;

	CoalesceAllocator coalesce_alloc;
};
