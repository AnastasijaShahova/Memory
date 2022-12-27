#pragma once

#include <cassert>
//#include <windows.h>

#ifdef _DEBUG
#include <iostream>
#endif 

#define INDEX_END_OF_LIST -1

class FixedSizeAllocator {
public:
	FixedSizeAllocator();
	virtual ~FixedSizeAllocator();

	virtual void init(size_t block_size, size_t num_blocks_page);
	virtual void destroy();

	virtual void* alloc(size_t size);
	virtual bool free(void* p);

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

	struct Page {
		Page* next;
		size_t fh; 
		size_t num_initialized;
		void* blocks;
	};

    void allocPage(Page*& page);
	void destroyPage(Page*& page);

	size_t block_size;
	size_t num_blocks_page;
	Page *page;
};