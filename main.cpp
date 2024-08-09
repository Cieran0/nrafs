#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <numeric>
#include <bitset>

const int BLOCK_SIZE = 256;
u_int8_t FILE_SYSTEM[16384] = {0};

struct FileHeader{
    bool free;
    char name[9];
    int8_t blocks[5];
    u_int8_t last_block_filled;
};

const int BLOCK_COUNT = (16384 - 256*sizeof(FileHeader)) / BLOCK_SIZE;

std::bitset<BLOCK_COUNT> block_bitmap; 

FileHeader empty_file() {
    char empty_name[9] = {'\0'};
    return (FileHeader){
        .free = true,
        .name = {'\0'},
        .blocks = {-1},
        .last_block_filled = 0
    };
}

void write_to_filesystem(u_int8_t* data, int location, int size) {
    std::memcpy((void*)(FILE_SYSTEM + location), data, size);
}

std::vector<int8_t> get_empty_blocks() {
    std::vector<int8_t> free_blocks;
    for (int8_t i = 0; i < BLOCK_COUNT; ++i) {
        if (!block_bitmap.test(i)) {
            free_blocks.push_back(i);
        }
    }
    return free_blocks;
}

FileHeader* get_next_free_file_header() {
    FileHeader* file_headers = (FileHeader*)FILE_SYSTEM;
    for (int i = 0; i < 256; i++)
    {
        if(file_headers[i].free)
            return file_headers + i;
    }
    return nullptr;
}

void make_file(char* name, uint8_t* data, int size) {
    int blocks_needed = size/BLOCK_SIZE + ((size % BLOCK_SIZE) != 0);
    int last_byte_of_last_block = uint8_t((size % BLOCK_SIZE) - 1);
    
    std::cout << "Blocks needed: " << blocks_needed << ", Last byte: " << last_byte_of_last_block << std::endl;

    if(blocks_needed > 5) {
        std::cout << "File too big" << std::endl;
    }

    std::vector<int8_t> blocks_avaible = get_empty_blocks();

    std::cout << "Blocks free: " << blocks_avaible.size() << std::endl;

    if(blocks_avaible.size() < blocks_needed) {
        std::cout << "Only " << blocks_avaible.size() << " blocks free! Failed to create file." << std::endl;
        return;
    }

    FileHeader* fh = get_next_free_file_header();
    int i =0;
    while (name != 0 && i < 9)
    {
        fh->name[i] = *name;
        name++;
        i++;
    }

    if(fh == nullptr) {
        std::cout << "No fileheader avaible" << std::endl;
        return;
    }

    for (int i = 0; i < blocks_needed; i++)
    {
        fh->blocks[i] = blocks_avaible[i];
        block_bitmap.set(blocks_avaible[i]);
        std::cout << "Block: " << ((int)blocks_avaible[i]) << " allocated" << std::endl;
    }
    if(blocks_needed != 5) {
        fh->blocks[blocks_needed] = -1;
    }
    fh->free = false;

    std::cout << "File : " << fh->name << " created!" << std::endl;
    
}

void load_bitmap() {
    block_bitmap.reset();

    FileHeader* file_headers = (FileHeader*)FILE_SYSTEM;

    for (int i = 0; i < 256; i++)
    {
        if(file_headers[i].free)
            break;

        for(int j =0; j<5; j++) {
            int block_id = file_headers[i].blocks[j];
            if(block_id == -1)
                break;
            block_bitmap.set(i);
            
        }
        
    }
}

FileHeader* find_file(const char* name) {
    FileHeader* file_headers = (FileHeader*)FILE_SYSTEM;

    for (int i = 0; i < 256; i++)
    {
        if(file_headers[i].free)
            continue;

        if (strcmp(file_headers[i].name, name) == 0) {
            return file_headers + i;
        }
    }

    return nullptr;
}

void delete_file(const char* name) {
    FileHeader* file = find_file(name);
    if(file == nullptr)
        return;
    file->free = true;

    for (int i = 0; i < 5; i++)
    {
        int block_id = file->blocks[i];
        if(block_id == -1)
            break;
        block_bitmap.reset(block_id);
    }
    memset(file->name, 9, 0);
    file->last_block_filled = 0;
    
}

int main() {

    std::ofstream fout;

    FileHeader file_system_header[256];

    for (int i = 0; i < 256; i++)
    {
        file_system_header[i] = empty_file();
    }

    std::cout << sizeof(file_system_header) << std::endl;

    write_to_filesystem((u_int8_t*)file_system_header, 0, sizeof(file_system_header));
    
    fout.open("file.bin", std::ios::binary | std::ios::out);
    fout.write((char*)FILE_SYSTEM, sizeof(FILE_SYSTEM));

    make_file("test1", nullptr, 256);
    make_file("test2", nullptr, 512);
    delete_file("test1");
    make_file("test3", nullptr, 555);
    make_file("test4", nullptr, 10);


    return 0;
}
