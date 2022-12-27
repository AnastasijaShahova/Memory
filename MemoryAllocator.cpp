#include "MemoryAllocator.h"

MemoryAllocator::MemoryAllocator() {
#ifdef _DEBUG
	is_initialized = false;
	is_destroyed = false;

	num_alloc = 0;
	num_free = 0;
#endif 
}

MemoryAllocator::~MemoryAllocator() {
#ifdef _DEBUG
	assert(is_destroyed && "MemoryAllocator: not destroyed before delete");
#endif 
}

void MemoryAllocator::init() {
#ifdef _DEBUG
	is_initialized = true;
	assert(!is_destroyed && "MemoryAllocator: not destroyed before init");
	is_destroyed = false;
#endif 
	fsa16.init(16, 512);
	fsa32.init(32, 256);
	fsa64.init(64, 128);
	fsa128.init(128, 64);
	fsa256.init(256, 32);
	fsa512.init(512, 16);

	coalesce_alloc.init(SIZE * 2);
}
void MemoryAllocator::destroy() {
#ifdef _DEBUG
	assert(is_initialized && "MemoryAllocator: not initialized before destroy");
	is_destroyed = true;
	is_initialized = false;
#endif 
	fsa16.destroy();
	fsa32.destroy();
	fsa64.destroy();
	fsa128.destroy();
	fsa256.destroy();
	fsa512.destroy();

	coalesce_alloc.destroy();

	for (size_t i = 0; i < OSBlocks.size(); i++) {
		VirtualFree(OSBlocks[i].data, 0, MEM_RELEASE);
	}
}

void* MemoryAllocator::alloc(size_t size) {
#ifdef _DEBUG
	assert(is_initialized && "MemoryAllocator: not initialized before alloc");
	num_alloc++;
#endif 

	if (size <= 16) {
		return fsa16.alloc(size);
	} 
	if (size <= 32) {
		return fsa32.alloc(size);
	}
	if (size <= 64) {
		return fsa64.alloc(size);
	}
	if (size <= 128) {
		return fsa128.alloc(size);
	}
	if (size <= 256) {
		return fsa256.alloc(size);
	}
	if (size <= 512) {
		return fsa512.alloc(size);
	}
	if (size < SIZE) {
		return coalesce_alloc.alloc(size);
	}
	void* p = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	Block block;
	block.data = p;
	block.size = size;
	OSBlocks.push_back(block);

	return p;
}
void MemoryAllocator::free(void* p) {

#ifdef _DEBUG
	assert(is_initialized && "MemoryAllocator: not initialized before free");
	num_free++;
#endif 

	if (fsa16.free(p)) {
		return;
	}
	if (fsa32.free(p)) {
		return;
	}
	if (fsa64.free(p)) {
		return;
	}
	if (fsa128.free(p)) {
		return;
	}
	if (fsa256.free(p)) {
		return;
	}
	if (fsa512.free(p)) {
		return;
	}

	if (coalesce_alloc.free(p)) {
		return;
	}

	bool result = VirtualFree(p, 0, MEM_RELEASE);
	assert(result && "Poiner out of bounds");

	if (result) {
		for (auto it = OSBlocks.begin(); it < OSBlocks.end(); it++) {
			if (static_cast<Block>(*it).data == p) {
				OSBlocks.erase(it);
				break;
			}
		}
	}
}

#ifdef _DEBUG
void MemoryAllocator::dumpStat() const {
	assert(is_initialized && "MemoryAllocator: not initialized before dumpStat");

	std::cout << "Memory Allocator:" << std::endl;
	std::cout << "\tAllocs: " << num_alloc << " Frees: " << num_free << std::endl;

	fsa16.dumpStat();
	fsa32.dumpStat();
	fsa64.dumpStat();
	fsa128.dumpStat();
	fsa256.dumpStat();
	fsa512.dumpStat();

	coalesce_alloc.dumpStat();

	std::cout << "\tOS Blocks:" << std::endl;
	for (size_t i = 0; i < OSBlocks.size(); i++) {
		std::cout << "\t\tBlock " << i << " Adress: " << OSBlocks[i].data << " Size: " << OSBlocks[i].size << std::endl;
	}
	std::cout << std::endl;
}
void MemoryAllocator::dumpBlocks() const {
	assert(is_initialized && "MemoryAllocator: not initialized before dumpBlocks");

	std::cout << "Memory Allocator:" << std::endl;
	fsa16.dumpBlocks();
	fsa32.dumpBlocks();
	fsa64.dumpBlocks();
	fsa128.dumpBlocks();
	fsa256.dumpBlocks();
	fsa512.dumpBlocks(); 

	coalesce_alloc.dumpBlocks();

	std::cout << "\tOS Blocks:" << std::endl;
	for (size_t i = 0; i < OSBlocks.size(); i++) {
		std::cout << "\t\tBlock " << i << " Adress: " << OSBlocks[i].data << " Size: " << OSBlocks[i].size << std::endl;
	}

	std::cout << std::endl;
}
#endif