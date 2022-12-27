#include "CoalesceAllocator.h"

CoalesceAllocator::CoalesceAllocator() {
#ifdef _DEBUG
    is_initialized = false;
    is_destroyed = false;

    num_alloc = 0;
    num_free = 0;
#endif 
}

CoalesceAllocator::~CoalesceAllocator() {
#ifdef _DEBUG
    assert(is_destroyed && "CoalesceAllocator: not destroyed before delete");
#endif 
}

void CoalesceAllocator::init(size_t OSBlockSize) {
#ifdef _DEBUG
    is_initialized = true;
    assert(!is_destroyed && "CoalesceAllocator: not destroyed before init");
    is_destroyed = false;
#endif 

    this->buffer_size = OSBlockSize;
    allocBuffer(buffer);
}

void CoalesceAllocator::destroy() {
#ifdef _DEBUG
    assert(is_initialized && "CoalesceAllocator: not initialized before destroy");
    is_destroyed = true;
    is_initialized = false;
#endif 

    destroyBuffer(buffer);
}

void* CoalesceAllocator::alloc(size_t size) {
#ifdef _DEBUG
    assert(is_initialized && "CoalesceAllocator: not initialized before alloc");
    num_alloc++;
#endif 

    Buffer* current_buff = buffer;
    while (true) {

        if (current_buff->fh != INDEX_END_OF_LIST) {

            Block* current_block = static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_buff->blocks) + current_buff->fh));
            Block* prev_block = nullptr;
            size_t free_list_index;

            while (true) {
                free_list_index = *static_cast<size_t*>(current_block->data);
                if (current_block->size >= size) {

                    if (current_block->size == size) {
                        if (prev_block == nullptr) {
                            current_buff->fh = free_list_index;
                        }
                        else {
                            *static_cast<size_t*>(prev_block->data) = INDEX_END_OF_LIST;
                        }
                        current_block->free = false;
                        return current_block->data;
                    }

                    size_t old_size = current_block->size;
                    current_block->size = size;
                    Block* new_block = static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_block->data) + size));
                    new_block->data = static_cast<char*>(static_cast<void*>(new_block)) + sizeof(Block);
                    new_block->size = old_size - size - sizeof(Block);
                    new_block->free = true;
                    *static_cast<size_t*>(new_block->data) = INDEX_END_OF_LIST;
                    if (prev_block == nullptr) {
                        current_buff->fh = static_cast<char*>(static_cast<void*>(new_block)) - static_cast<char*>(current_buff->blocks);
                    }
                    else {
                        *static_cast<size_t*>(prev_block->data) = static_cast<char*>(static_cast<void*>(new_block)) - static_cast<char*>(current_buff->blocks);
                    }
                    current_block->free = false;
                    return current_block->data;
                }

                if (free_list_index == INDEX_END_OF_LIST) {
                    break;
                }

                prev_block = current_block;
                current_block = static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_buff->blocks) + free_list_index));
            }
        }
    
        if (current_buff->next == nullptr) {
            allocBuffer(current_buff->next);
        }
        current_buff = current_buff->next;
    }
}


bool CoalesceAllocator::free(void* p) {
#ifdef _DEBUG
    assert(is_initialized && "CoalesceAllocator: not initialized before free");
    num_free++;
#endif 

    Buffer* current_buff = buffer;
    while (current_buff != nullptr) {

        if (current_buff->blocks < p && (char*)current_buff->blocks + buffer_size + sizeof(Block) > p) {
            Block* current_block = static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_buff->blocks)));
            Block* prev_block = nullptr;

            while (current_block->data < p) {
                prev_block = current_block;
                current_block = static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_block->data) + current_block->size));
            }

            Block* next_block = static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_block->data) + current_block->size));

            if (static_cast<void*>(next_block) >= (char*)current_buff->blocks + buffer_size + sizeof(Block)) {
                next_block = nullptr;
            }

            if (next_block != nullptr && next_block->free) {
                size_t next_block_fl_index = current_buff->fh;
                size_t prev_fl_index = INDEX_END_OF_LIST;
                while (static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_buff->blocks) + next_block_fl_index)) != next_block) {
                    prev_fl_index = next_block_fl_index;
                    next_block_fl_index = *static_cast<size_t*>(static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_buff->blocks) + next_block_fl_index))->data);
                }

                if (prev_fl_index == INDEX_END_OF_LIST) { 
                    current_buff->fh = *static_cast<size_t*>(next_block->data);
                }
                else {
                    *static_cast<size_t*>(static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_buff->blocks) + prev_fl_index))->data) =
                        *static_cast<size_t*>(next_block->data);;
                }

                current_block->size += next_block->size + sizeof(Block);
            }

            if (prev_block != nullptr && prev_block->free) {
                prev_block->size += current_block->size + sizeof(Block);
                return true;
            }

            current_block->free = true;
            *static_cast<size_t*>(current_block->data) = current_buff->fh;
            current_buff->fh = static_cast<char*>(static_cast<void*>(current_block)) - static_cast<char*>(current_buff->blocks);
            
            return true;
        }
        current_buff = current_buff->next;
    }

#ifdef _DEBUG
    num_free--;
#endif 
    return false;
}

#ifdef _DEBUG
void CoalesceAllocator::dumpStat() const {
    assert(is_initialized && "CoalesceAllocator: not initialized before dumpStat");

    std::cout << "\tCoalesceAllocator:" << std::endl;
    std::cout << "\t\tAllocs: " << num_alloc << " Frees: " << num_free << std::endl;

    Buffer* current_buff = buffer;
    size_t busy_blocks = 0;
    size_t free_blocks = 0;

    while (current_buff != nullptr) {
        Block* current_block = static_cast<Block*>(current_buff->blocks);
        while (static_cast<char*>(static_cast<void*>(current_block)) - static_cast<char*>(current_buff->blocks) < buffer_size + sizeof(Block)) {
            
            if (current_block->free) {
                free_blocks++;
            }
            else {
                busy_blocks++;
            }

            current_block = static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_block->data) + current_block->size));
        }
        current_buff = current_buff->next;
    }

    std::cout << "\t\tBusy Blocks: " << busy_blocks << " Free Blocks: " << free_blocks << std::endl;

    std::cout << "\t\tOS Buffers:" << std::endl;
    current_buff = buffer;
    size_t buffer_index = 0;
    while (current_buff != nullptr) {
        std::cout << "\t\t\tBuffer " << buffer_index << " Adress: " << static_cast<void*>(current_buff) 
            << " Size: " << buffer_size + sizeof(Buffer) + sizeof(Block) << std::endl;
        buffer_index++;
        current_buff = current_buff->next;
    }
}


void CoalesceAllocator::dumpBlocks() const {
    assert(is_initialized && "CoalesceAllocator: not initialized before dumpBlocks");

    std::cout << "\tCoalesceAllocator:" << std::endl;

    Buffer* current_buff = buffer;
    size_t buffer_index = 0;

    while (current_buff != nullptr) {
        std::cout << "\t\tBuffer " << buffer_index << std::endl;
        Block* current_block = static_cast<Block*>(current_buff->blocks);
        size_t block_index = 0;
        while (static_cast<char*>(static_cast<void*>(current_block)) - static_cast<char*>(current_buff->blocks) < buffer_size + sizeof(Block)) {

            std::cout << "\t\t\tBlock " << block_index;
            if (current_block->free) {
                std::cout << " Free";
            }
            else {
                std::cout << " Busy";
            }

            block_index++;

            std::cout << " Adress: " << static_cast<void*>(current_block) << " Size " << current_block->size << std::endl;

            current_block = static_cast<Block*>(static_cast<void*>(static_cast<char*>(current_block->data) + current_block->size));
        }
        current_buff = current_buff->next;
        buffer_index++;
    }
}
#endif

void CoalesceAllocator::allocBuffer(Buffer*& buffer)
{
    void* buf = VirtualAlloc(NULL, buffer_size + sizeof(Buffer) + sizeof(Block), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    buffer = static_cast<Buffer*>(buf);
    buffer->next = nullptr;
    buffer->fh = 0;
    buffer->blocks = static_cast<char*>(buf) + sizeof(Buffer);
    Block* b = static_cast<Block*>(buffer->blocks);
    b->size = buffer_size;
    b->data = static_cast<char*>(static_cast<void*>(b)) + sizeof(Block);
    b->free = true;
    *static_cast<size_t*>(b->data) = INDEX_END_OF_LIST;
}

void CoalesceAllocator::destroyBuffer(Buffer*& buffer)
{
    if (buffer == nullptr) {
        return;
    }
    destroyBuffer(buffer->next);
    VirtualFree(static_cast<void*>(buffer), 0, MEM_RELEASE);
}