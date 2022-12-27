#include "FixedSizeAllocator.h"

FixedSizeAllocator::FixedSizeAllocator() {
#ifdef _DEBUG
	is_initialized = false;
	is_destroyed = false;

	num_alloc = 0;
	num_free = 0;
#endif 
}

FixedSizeAllocator::~FixedSizeAllocator() {
#ifdef _DEBUG
    assert(is_destroyed && "FSA: not destroyed before delete");
#endif 
}

void FixedSizeAllocator::init(size_t block_size,size_t num_blocks_page) {

#ifdef _DEBUG
    is_initialized = true;
    assert(!is_destroyed && "FSA: not destroyed before init");
    is_destroyed = false;
#endif 

    this->block_size = block_size;
    this->num_blocks_page = num_blocks_page;
    allocPage(page);
}

void FixedSizeAllocator::destroy() {

#ifdef _DEBUG
    assert(is_initialized && "FSA: not initialized before destroy");
    is_destroyed = true;
    is_initialized = false;
#endif 

    destroyPage(page);
    page = nullptr;
}

void* FixedSizeAllocator::alloc(size_t size) {

#ifdef _DEBUG
    assert(is_initialized && "FSA: not initialized before alloc");
    num_alloc++;
#endif 

    Page* current_page = page;

    while (current_page->fh == INDEX_END_OF_LIST) {
        if (current_page->num_initialized < num_blocks_page) {
            current_page->num_initialized++;
            void* p = static_cast<char*>(current_page->blocks) + (current_page->num_initialized - 1) * block_size;
            return p;
        }
        if (current_page->next == nullptr) {
            allocPage(current_page->next);
        }
        current_page = current_page->next;
    }
    void* p = static_cast<char*>(current_page->blocks) + current_page->fh * block_size;
    current_page->fh = *static_cast<size_t*>(p);
    return p;
}

bool FixedSizeAllocator::free(void* p) {

#ifdef _DEBUG
    assert(is_initialized && "FSA: not initialized before free");
    num_free++;
#endif

    Page* current_page = page;
    while (current_page != nullptr) {
        if (static_cast<void*>(current_page->blocks) <= p && static_cast<void*>(static_cast<char*>(current_page->blocks) + num_blocks_page * block_size) > p) {
            *static_cast<size_t*>(p) = current_page->fh;
            current_page->fh = static_cast<size_t>((static_cast<char*>(p) - static_cast<char*>(current_page->blocks))/block_size);

            return true;
        }
        current_page = current_page->next;
    }
#ifdef _DEBUG
    num_free--;
#endif 
    return false;
}

#ifdef _DEBUG
void FixedSizeAllocator::dumpStat() const {

    assert(is_initialized && "FSA: not initialized before dumpStat");
    std::cout << "\tFSA " << block_size << ":" << std::endl;
    std::cout << "\t\tAllocs: " << num_alloc << " Frees: " << num_free << std::endl;

    Page* current_page = page;
    size_t num_pages = 0;
    size_t busy_blocks = 0;
    size_t free_blocks = 0;

    while (current_page != nullptr) {

        for (size_t i = 0; i < current_page->num_initialized; i++) {

            bool is_free = false;
            size_t index = current_page->fh;

            while (index != INDEX_END_OF_LIST) {
                if (i == index) {
                    is_free = true;
                    break;
                }
                index = *static_cast<size_t*>(static_cast<void*>(static_cast<char*>(current_page->blocks) + index * block_size));
            }
            if (!is_free) {
                busy_blocks++;
            }
            else {
                free_blocks++;
            }
        }
        current_page = current_page->next;
        num_pages++;
    }

    std::cout << "\t\tPages: " << num_pages;
    std::cout << " Busy Blocks: " << busy_blocks << " Free Blocks: " << free_blocks << std::endl;
    std::cout << "\t\tOS Buffers:" << std::endl;

    current_page = page;
    size_t page_index = 0;

    while (current_page != nullptr) {
        std::cout << "\t\t\tBuffer " << page_index << " Adress: " << static_cast<void*>(current_page) 
            << " Size: " << block_size * num_blocks_page + sizeof(Page) << std::endl;
        page_index++;
        current_page = current_page->next;
    }
}

void FixedSizeAllocator::dumpBlocks() const {
    assert(is_initialized && "FSA: not initialized before dumpBlocks");
    std::cout << "\tFSA " << block_size << ":" << std::endl;
    Page* current_page = page;
    size_t page_index = 0;

    while (current_page != nullptr) {

        std::cout << "\t\tPage " << page_index << std::endl;

        for (size_t i = 0; i < current_page->num_initialized; i++) {
            bool is_free = false; 
            size_t index = current_page->fh;

            while (index != INDEX_END_OF_LIST) {
                if (i == index) {
                    is_free = true;
                    break;
                }
                index = *static_cast<size_t*>(static_cast<void*>(static_cast<char*>(current_page->blocks) + index * block_size));
            }
            std::cout << "\t\t\tBlock " << i;

            if (!is_free) {
                std::cout << " Busy";
            }
            else {
                std::cout << " Free";
            }

            std::cout << " Adress: " << static_cast<void*>(static_cast<char*>(current_page->blocks) + i * block_size) << std::endl;
        }

        current_page = current_page->next;
        page_index++;
    }
}
#endif 

void FixedSizeAllocator::allocPage(Page*& page) {

    void* buf = VirtualAlloc(NULL, block_size * num_blocks_page + sizeof(Page), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    page = static_cast<Page*>(buf);
    page->next = nullptr;
    page->fh = INDEX_END_OF_LIST;
    page->blocks = static_cast<char*>(buf) + sizeof(Page);
    page->num_initialized = 0;
}

void FixedSizeAllocator::destroyPage(Page*& page) {

    if (page == nullptr) {
        return;
    }

    destroyPage(page->next);
    VirtualFree(static_cast<void*>(page), 0, MEM_RELEASE);
}