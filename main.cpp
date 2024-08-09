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

struct Block {
    uint8_t bytes[256];
};

struct FileHeader{
    bool free;
    char name[9];
    int8_t blocks[5];
    u_int8_t last_block_filled;
};

Block* BLOCKS = (Block*) ((void*)(FILE_SYSTEM + (256*sizeof(FileHeader))));

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

    for (int i = 0; i < blocks_needed; i++)
    {
        int length = (i == blocks_needed - 1)? (last_byte_of_last_block + 1) : BLOCK_SIZE;
        std::memcpy((void*)(BLOCKS + fh->blocks[i]), (void*)data, length);
    }
    
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

uint8_t* read_file_to_uint(char* filePath, int* fileSize) {

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cout << "Error opening file: " << filePath << std::endl;
        return nullptr;
    }

    *fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    uint8_t* buffer = new uint8_t[*fileSize];

    if (!file.read((char*)buffer, *fileSize)) {
        std::cout << "Error reading file: " << filePath << std::endl;
        return nullptr;
    }

    file.close();
    return buffer;
}

int get_blocks_used(FileHeader* fh) {

    for (int i = 0; i < 5; i++)
    {
        if(fh->blocks[i] == -1) {
            return i;
        }
    }

    return 5;
}

int get_file_size(FileHeader* fh) {
    return ((get_blocks_used(fh) - 1)*BLOCK_SIZE) + (fh->last_block_filled + 1);
}

uint8_t* read_file(FileHeader* fh, int* size) {
    *size = get_file_size(fh);
    int blocks_used = get_blocks_used(fh);

    uint8_t* file = (uint8_t*)malloc(*size);

    for (int i = 0; i < blocks_used-1; i++)
    {
        std::memcpy(file + (i*BLOCK_SIZE), BLOCKS + fh->blocks[i], BLOCK_SIZE);
    }

    std::memcpy(file + (blocks_used*BLOCK_SIZE), BLOCKS + fh->blocks[blocks_used], fh->last_block_filled+1);

    return file;
}

void export_file(FileHeader* fh, char* path) {

    int size;
    uint8_t* file = read_file(fh, &size);

    std::ofstream fout;
    fout.open(path, std::ios::binary | std::ios::out);
    fout.write((char*)file, size);

    free(file);
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

    int fileSize;

    uint8_t* data = read_file_to_uint("lol.png", &fileSize);

    make_file("test1", data, fileSize);
    //delete_file("test1");
    //make_file("test3", nullptr, 555);
    //make_file("test4", nullptr, 10);
    FileHeader* fh = find_file("test1");
    export_file(fh, "lol2.png");
    uint8_t buff[fileSize];


    return 0;
}
