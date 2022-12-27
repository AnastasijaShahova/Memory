#pragma once
#include <cassert>
//#include <windows.h>

#ifdef _DEBUG
#include <iostream>
#endif 

#define INDEX_END_OF_LIST -1

class CoalesceAllocator {
public:
	CoalesceAllocator();

	virtual ~CoalesceAllocator();

	virtual void init(size_t OSBlockSize);
	virtual void destroy();

	virtual void* alloc(size_t size);
	virtual bool free(void* p);

#ifdef _DEBUG
	virtual void dumpStat() const;
	virtual void dumpBlocks() const;
#endif

private:
	struct Buffer {
		Buffer* next;
		size_t fh;
		void* blocks;
	};
	struct Block {
		size_t size;
		void* data;
		bool free;
	};

	void allocBuffer(Buffer*& buffer);
	void destroyBuffer(Buffer*& buffer);

    size_t buffer_size;
	Buffer* buffer;

#ifdef _DEBUG
	bool is_initialized;
	bool is_destroyed;

	size_t num_alloc;
	size_t num_free;
#endif  
};