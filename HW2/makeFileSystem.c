#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MAX_FILES 128
#define FAT_ENTRIES (4 * 1024) // 4*1024 FAT entries for a 4MB file system

#define PERMISSION_READ 0x01
#define PERMISSION_WRITE 0x02
#define PERMISSION_RW (PERMISSION_READ | PERMISSION_WRITE)

typedef struct {
    uint8_t name[255]; // File name variable length
    uint32_t size; // Size of the file
    uint8_t permissions; // Permissions for R and W
    time_t creation_time; // Creation date and time
    time_t modification_time; // Last modification date and time
    char password[20]; // Password for protection
    uint16_t first_block; // Index of the first data block
} DirectoryEntry;

typedef struct {
    DirectoryEntry directory[MAX_FILES]; // Directory entries
    uint32_t directory_count; // Directory count
    uint16_t fat[FAT_ENTRIES]; // FAT table
    uint32_t block_size; // Block size
    uint32_t total_blocks; // Total number of blocks
    uint32_t free_blocks; // Number of free blocks
} FileSystem; // Super block

void print_super_block(const char *filename);
void read_block(const char *filename, int block_number, int block_size);
void initialize_file_system(const char *filename, uint32_t block_size, uint32_t file_system_size);

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Block size must be 1 or 0.5 KB\n");
        return EXIT_FAILURE;
    }

    float block_size_kb = atof(argv[1]);
    const char *filename = argv[2];

    if (block_size_kb != 1.0 && block_size_kb != 0.5) {
        fprintf(stderr, "Block size must be 1 or 0.5 KB\n");
        return EXIT_FAILURE;
    }

    uint32_t block_size_bytes = (uint32_t)(block_size_kb * 1024);
    uint32_t file_system_size = (block_size_kb == 1.0) ? (4 * 1024 * 1024) : (2 * 1024 * 1024);
    initialize_file_system(filename, block_size_bytes, file_system_size);

    // Verify the block size by reading the first block
    if (argc == 4) {
        int block_number = atoi(argv[3]);
        read_block(filename, block_number, block_size_bytes);
    }

    return EXIT_SUCCESS;
}


// Function to print the super block information
void print_super_block(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open file system");
        exit(EXIT_FAILURE);
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);
    fclose(fp);

    printf("Super Block Information:\n");
    printf("Block size: %u bytes\n", fs.block_size);
    printf("Total blocks: %u\n", fs.total_blocks);
    printf("Free blocks: %u\n", fs.free_blocks);
    printf("Directory count: %u\n", fs.directory_count);
    
    printf("\nFAT Table (First 10 Entries):\n");
    for (int i = 0; i < 10; i++) {
        printf("FAT[%d]: %u\n", i, fs.fat[i]);
    }

    printf("\nDirectory Entries:\n");
    for (uint32_t i = 0; i < fs.directory_count; i++) {
        printf("File %u: Name: %s, Size: %u, Permissions: %c%c\n",
               i, fs.directory[i].name, fs.directory[i].size,
               (fs.directory[i].permissions & PERMISSION_READ) ? 'r' : '-',
               (fs.directory[i].permissions & PERMISSION_WRITE) ? 'w' : '-');
    }
}

// Function to read and print the contents of a specific block
void read_block(const char *filename, int block_number, int block_size) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open file system");
        exit(EXIT_FAILURE);
    }

    uint8_t *buffer = (uint8_t*)malloc(block_size);
    fseek(fp, block_number * block_size, SEEK_SET);
    fread(buffer, 1, block_size, fp);
    fclose(fp);

    printf("Block %d contents:\n", block_number);
    for (int i = 0; i < block_size; i++) {
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");

    free(buffer);
}

// Function to initialize a file system with specified block size and total size
void initialize_file_system(const char *filename, uint32_t block_size, uint32_t file_system_size) {
    FileSystem fs;
    memset(&fs, 0, sizeof(FileSystem));

    for (int i = 0; i < FAT_ENTRIES; i++) {
        fs.fat[i] = 0xFFFF; // Use 0xFFFF to indicate that blocks are free.
    }

    fs.block_size = block_size;
    fs.total_blocks = file_system_size / block_size;
    fs.free_blocks = fs.total_blocks;
    fs.directory_count = 0;

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Failed to create file system");
        exit(EXIT_FAILURE);
    }

    fwrite(&fs, sizeof(FileSystem), 1, fp);

    uint8_t zero = 0;
    for (size_t i = sizeof(FileSystem); i < file_system_size; i++) {
        fwrite(&zero, 1, 1, fp);
    }

    fclose(fp);
    printf("File system created: %s, Block size: %u bytes\n", filename, block_size);

    // Check the super block
    //print_super_block(filename);
}

