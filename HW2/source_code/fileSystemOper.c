#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MAX_FILES 128
#define BLOCK_SIZE 1024
#define FAT_ENTRIES (4 * 1024)
#define MAX_FILENAME_LENGTH 255
#define MAX_PASSWORD_LENGTH 20

#define PERMISSION_READ 0x01
#define PERMISSION_WRITE 0x02
#define PERMISSION_RW (PERMISSION_READ | PERMISSION_WRITE)

typedef struct {
    uint8_t name[MAX_FILENAME_LENGTH]; // File name (variable length)
    uint32_t size; // Size of the file
    uint8_t permissions; // Permissions (R and W)
    time_t creation_time; // Creation date and time
    time_t modification_time; // Last modification date and time
    char password[MAX_PASSWORD_LENGTH]; // Password for protection (if any)
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

void read_block(const char *filename, int block_number, int block_size);
void write_file(const char *filesystem_name, const char *path, const char *linux_file);
void read_file(const char *filesystem_name, const char *path, const char *linux_file, const char *password);
void list_directory(const char *filesystem_name, const char *path);
int directory_exists(FileSystem *fs, const char *path);
void make_directory(const char *filesystem_name, const char *path);
void delete_file(const char *filesystem_name, const char *path, const char *password);
void remove_directory(const char *filesystem_name, const char *path);
void change_permissions(const char *filesystem_name, const char *path, const char *permissions, const char *password);
void add_password(const char *filesystem_name, const char *path, const char *password);
void dump_file_system(const char *filesystem_name);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Invalid arguments\n");
        return EXIT_FAILURE;
    }

    const char *fs_name = argv[1];
    const char *operation = argv[2];

    if (strcmp(operation, "dir") == 0) {
        if (argc != 4) {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
        list_directory(fs_name, argv[3]);
    } else if (strcmp(operation, "mkdir") == 0) {
        if (argc != 4) {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
        make_directory(fs_name, argv[3]);
    } else if (strcmp(operation, "rmdir") == 0) {
        if (argc != 4) {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
        remove_directory(fs_name, argv[3]);
    } else if (strcmp(operation, "dumpe2fs") == 0) {
        if (argc != 3) {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
        dump_file_system(fs_name);
    } else if (strcmp(operation, "write") == 0) {
        if (argc != 5) {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
        write_file(fs_name, argv[3], argv[4]);
    } else if (strcmp(operation, "read") == 0) {
        if (argc == 5) {
            read_file(fs_name, argv[3], argv[4], "");
        } else if (argc == 6) {
            read_file(fs_name, argv[3], argv[4], argv[5]);
        } else {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
    } else if (strcmp(operation, "del") == 0) {
        if (argc == 4) {
            delete_file(fs_name, argv[3], "");
        } else if (argc == 5) {
            delete_file(fs_name, argv[3], argv[4]);
        } else {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
    } else if (strcmp(operation, "chmod") == 0) {
        if (argc == 5) {
            change_permissions(fs_name, argv[3], argv[4], "");
        } else if (argc == 6) {
            change_permissions(fs_name, argv[3], argv[4], argv[5]);
        } else {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
    } else if (strcmp(operation, "addpw") == 0) {
        if (argc != 5) {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
        add_password(fs_name, argv[3], argv[4]);
    } else if (strcmp(operation, "read_block_content") == 0) {
        if (argc != 5) {
            printf("Invalid arguments\n");
            return EXIT_FAILURE;
        }
        int block_number = atoi(argv[4]);
        read_block(fs_name, block_number, BLOCK_SIZE);
    } else {
        fprintf(stderr, "Invalid operation: %s\n", operation);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Function to read the content of a specific block
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

    printf("Content of block %d:\n", block_number);
    for (int i = 0; i < block_size; i++) {
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");

    free(buffer);
}

// Function to write a file to the file system
void write_file(const char *filesystem_name, const char *path, const char *linux_file) {
    FILE *fp = fopen(filesystem_name, "rb+");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);

    // Check if the directory exists
    char dir_path[MAX_FILENAME_LENGTH];
    strncpy(dir_path, path, sizeof(dir_path));
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0'; // Separate the directory part
    }

    int dir_exists = 0;
    for (uint32_t i = 0; i < fs.directory_count; i++) {
        if (strcmp((char *)fs.directory[i].name, dir_path) == 0) {
            dir_exists = 1;
            break;
        }
    }

    if (!dir_exists) {
        printf("Subdirectory does not exist: %s\n", dir_path);
        fclose(fp);
        return;
    }

    // Check for permissions and existence before adding the file
    for (uint32_t i = 0; i < fs.directory_count; i++) {
        if (strcmp((char *)fs.directory[i].name, path) == 0) {
            if (!(fs.directory[i].permissions & PERMISSION_WRITE)) { // Check write permission first
                printf("No write permission for file: %s\n", path);
                fclose(fp);
                return;
            }
            printf("File already exists: %s\n", path);
            fclose(fp);
            return;
        }
    }

    FILE *lfp = fopen(linux_file, "rb");
    if (!lfp) {
        perror("Failed to open Linux file");
        fclose(fp);
        return;
    }

    fseek(lfp, 0, SEEK_END);
    long file_size = ftell(lfp);
    fseek(lfp, 0, SEEK_SET);

    uint8_t *buffer = malloc(file_size);
    fread(buffer, 1, file_size, lfp);
    fclose(lfp);

    DirectoryEntry new_file;
    strncpy((char*)new_file.name, path, sizeof(new_file.name) - 1);
    new_file.size = file_size;
    new_file.permissions = PERMISSION_RW; // Read and write permissions
    new_file.creation_time = time(NULL);
    new_file.modification_time = new_file.creation_time;
    memset(new_file.password, 0, sizeof(new_file.password));

    // Find empty blocks in the FAT table and place the data
    uint16_t prev_block = 0xFFFF;
    uint16_t first_block = 0xFFFF;
    size_t written_size = 0;
    int used_blocks = 0;

    int found = 0;
    for (uint16_t i = 0; i < FAT_ENTRIES; i++) {
        if (fs.fat[i] == 0xFFFF) { // Empty block found
            if (first_block == 0xFFFF) {
                first_block = i;
            }
            if (prev_block != 0xFFFF) {
                fs.fat[prev_block] = i; // Link the previous block to the next block
            }
            prev_block = i;

            fseek(fp, sizeof(FileSystem) + i * fs.block_size, SEEK_SET);
            size_t to_write = (file_size - written_size) < fs.block_size ? (file_size - written_size) : fs.block_size;
            fwrite(buffer + written_size, 1, to_write, fp);
            written_size += to_write;
            used_blocks++;
            
            if (written_size >= file_size) {
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        fprintf(stderr, "Not enough empty blocks found\n");
        free(buffer);
        fclose(fp);
        return;
    }

    fs.fat[prev_block] = 0xFFF; // End of file marker

    new_file.first_block = first_block;
    fs.directory[fs.directory_count++] = new_file;

    // Update the file system
    fs.free_blocks -= used_blocks;
    fseek(fp, 0, SEEK_SET);
    fwrite(&fs, sizeof(FileSystem), 1, fp);
    fclose(fp);

    free(buffer);
    
    // Inform the user after the file is successfully written
    printf("File successfully written: %s\n", path);
}

// Function to read a file from the file system
void read_file(const char *filesystem_name, const char *path, const char *linux_file, const char *password) {
    FILE *fp = fopen(filesystem_name, "rb");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);

    DirectoryEntry *entry = NULL;
    for (uint32_t i = 0; i < fs.directory_count; i++) {
        if (strcmp((char*)fs.directory[i].name, path) == 0) {
            entry = &fs.directory[i];
            break;
        }
    }

    if (!entry) {
        fclose(fp);
        printf("File not found: %s\n", path);
        return;
    }

    // Password check
    if (entry->password[0] != '\0' && (password == NULL || strcmp(entry->password, password) != 0)) {
        fclose(fp);
        printf("Incorrect password: %s\n", path);
        return;
    }

    // Permission check
    if (!(entry->permissions & PERMISSION_READ)) {
        fclose(fp);
        printf("No read permission for file: %s\n", path);
        return;
    }

    FILE *lfp = fopen(linux_file, "wb");
    if (!lfp) {
        perror("Failed to open Linux file");
        fclose(fp);
        return;
    }

    uint8_t *buffer = malloc(entry->size);
    size_t read_size = 0;
    uint16_t current_block = entry->first_block;

    while (current_block != 0xFFF && read_size < entry->size) {
        fseek(fp, sizeof(FileSystem) + current_block * BLOCK_SIZE, SEEK_SET);
        size_t to_read = (entry->size - read_size) < BLOCK_SIZE ? (entry->size - read_size) : BLOCK_SIZE;
        fread(buffer + read_size, 1, to_read, fp);
        read_size += to_read;
        current_block = fs.fat[current_block];
    }

    fwrite(buffer, 1, read_size, lfp);
    fclose(lfp);
    free(buffer);
    fclose(fp);
    printf("File read: %s\n", path);
}

// Function to list the directory content
// Function to list the directory content
void list_directory(const char *filesystem_name, const char *path) {
    FILE *fp = fopen(filesystem_name, "rb");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);
    fclose(fp);

    int directory_found = 0;

    printf("Directory content: %s\n", path);
    for (uint32_t i = 0; i < fs.directory_count; i++) {
        DirectoryEntry entry = fs.directory[i];

        if (strncmp((char*)entry.name, path, strlen(path)) == 0) {
            directory_found = 1;
            printf("Name: %s, Size: %u, Permissions: %c%c",
                   entry.name, entry.size,
                   (entry.permissions & PERMISSION_READ) ? 'r' : '-',
                   (entry.permissions & PERMISSION_WRITE) ? 'w' : '-');
            if (entry.password[0] != '\0') {
                printf(", Password protected: yes\n");
            } else {
                printf(", Password protected: no\n");
            }
        }
    }

    if (!directory_found) {
        printf("Directory not found: %s\n", path);
    }
}

// Function to check if a directory exists
int directory_exists(FileSystem *fs, const char *path) {
    for (uint32_t i = 0; i < fs->directory_count; i++) {
        if (strcmp((char*)fs->directory[i].name, path) == 0) {
            return 1;
        }
    }
    return 0;
}

// Function to create a new directory
void make_directory(const char *filesystem_name, const char *path) {
    FILE *fp = fopen(filesystem_name, "rb+");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);

    // Check if the directory already exists
    if (directory_exists(&fs, path)) {
        fclose(fp);
        printf("Directory already exists: %s\n", path);
        return;
    }

    // Check for valid parent directory
    char parent_path[MAX_FILENAME_LENGTH];
    strncpy(parent_path, path, sizeof(parent_path) - 1);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash != NULL && last_slash != parent_path) {
        *last_slash = '\0';
        if (!directory_exists(&fs, parent_path)) {
            fclose(fp);
            printf("Parent directory does not exist: %s\n", parent_path);
            return;
        }
    } else if (last_slash == NULL) {
        // Invalid path check
        fclose(fp);
        printf("Invalid path: %s\n", path);
        return;
    }

    // Create new directory entry
    DirectoryEntry new_dir;
    strncpy((char*)new_dir.name, path, sizeof(new_dir.name) - 1);
    new_dir.size = 0;
    new_dir.permissions = PERMISSION_RW; // Read and write permissions
    new_dir.creation_time = time(NULL);
    new_dir.modification_time = new_dir.creation_time;
    memset(new_dir.password, 0, sizeof(new_dir.password));
    new_dir.first_block = 0xFFFF;

    fs.directory[fs.directory_count++] = new_dir;

    // Update the file system
    fseek(fp, 0, SEEK_SET);
    fwrite(&fs, sizeof(FileSystem), 1, fp);
    fclose(fp);

    printf("Directory created: %s\n", path);
}

// Function to delete a file from the file system
void delete_file(const char *filesystem_name, const char *path, const char *password) {
    FILE *fp = fopen(filesystem_name, "rb+");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);

    for (uint32_t i = 0; i < fs.directory_count; i++) {
        if (strcmp((char*)fs.directory[i].name, path) == 0) {
            // Check if it is a directory
            if (fs.directory[i].size == 0) {
                printf("Error: '%s' is a directory. Please use 'rmdir' command.\n", path);
                fclose(fp);
                return;
            }

            // Password check
            if (fs.directory[i].password[0] != '\0' && (password == NULL || strcmp(fs.directory[i].password, password) != 0)) {
                fclose(fp);
                printf("Incorrect password: %s\n", path);
                return;
            }

            // Release blocks in the FAT table and update the free block count
            uint16_t current_block = fs.directory[i].first_block;
            uint32_t blocks_freed = 0;
            while (current_block != 0xFFF) {
                uint16_t next_block = fs.fat[current_block];
                fs.fat[current_block] = 0xFFFF;
                current_block = next_block;
                blocks_freed++;
            }

            // Remove the file directory entry
            for (uint32_t j = i; j < fs.directory_count - 1; j++) {
                fs.directory[j] = fs.directory[j + 1];
            }
            fs.directory_count--;
            fs.free_blocks += blocks_freed; // Add the freed blocks to the free block count

            fseek(fp, 0, SEEK_SET);
            fwrite(&fs, sizeof(FileSystem), 1, fp);
            fclose(fp);
            printf("File deleted: %s\n", path);
            return;
        }
    }

    fclose(fp);
    printf("File not found: %s\n", path);
}

// Function to remove a directory from the file system
void remove_directory(const char *filesystem_name, const char *path) {
    FILE *fp = fopen(filesystem_name, "rb+");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);

    int dir_index = -1;
    for (uint32_t i = 0; i < fs.directory_count; i++) {
        if (strcmp((char*)fs.directory[i].name, path) == 0) {
            dir_index = i;
            break;
        }
    }

    if (dir_index == -1) {
        fclose(fp);
        printf("Directory not found: %s\n", path);
        return;
    }

    // Check if the directory is empty
    for (uint32_t i = 0; i < fs.directory_count; i++) {
        if (strncmp((char*)fs.directory[i].name, path, strlen(path)) == 0 && 
            fs.directory[i].name[strlen(path)] == '/') {
            fclose(fp);
            printf("Error: Directory is not empty: %s\n", path);
            return;
        }
    }

    // If the directory is empty, remove it
    uint16_t current_block = fs.directory[dir_index].first_block;
    while (current_block != 0xFFFF) {
        uint16_t next_block = fs.fat[current_block];
        fs.fat[current_block] = 0xFFFF;
        current_block = next_block;
    }

    for (uint32_t i = dir_index; i < fs.directory_count - 1; i++) {
        fs.directory[i] = fs.directory[i + 1];
    }
    fs.directory_count--;

    fseek(fp, 0, SEEK_SET);
    fwrite(&fs, sizeof(FileSystem), 1, fp);
    fclose(fp);

    printf("Directory removed: %s\n", path);
}

// Function to change file permissions
void change_permissions(const char *filesystem_name, const char *path, const char *permissions, const char *password) {
    FILE *fp = fopen(filesystem_name, "rb+");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);

    for (uint32_t i = 0; i < fs.directory_count; i++) {
        if (strcmp((char*)fs.directory[i].name, path) == 0) {
            // Password check
            if (fs.directory[i].password[0] != '\0' && (password == NULL || strcmp(fs.directory[i].password, password) != 0)) {
                fclose(fp);
                printf("Incorrect password: %s\n", path);
                return;
            }

            if (permissions[0] == '+') {
                if (strchr(permissions, 'r')) {
                    fs.directory[i].permissions |= PERMISSION_READ; // Add read permission
                }
                if (strchr(permissions, 'w')) {
                    fs.directory[i].permissions |= PERMISSION_WRITE; // Add write permission
                }
            } else if (permissions[0] == '-') {
                if (strchr(permissions, 'r')) {
                    fs.directory[i].permissions &= ~PERMISSION_READ; // Remove read permission
                }
                if (strchr(permissions, 'w')) {
                    fs.directory[i].permissions &= ~PERMISSION_WRITE; // Remove write permission
                }
            } else if (permissions[0] == 'r' || permissions[0] == 'w') {
                if (strchr(permissions, 'r')) {
                    fs.directory[i].permissions |= PERMISSION_READ; // Add read permission
                } else {
                    fs.directory[i].permissions &= ~PERMISSION_READ; // Remove read permission
                }
                if (strchr(permissions, 'w')) {
                    fs.directory[i].permissions |= PERMISSION_WRITE; // Add write permission
                } else {
                    fs.directory[i].permissions &= ~PERMISSION_WRITE; // Remove write permission
                }
            }
            fseek(fp, 0, SEEK_SET);
            fwrite(&fs, sizeof(FileSystem), 1, fp);
            fclose(fp);
            printf("Permissions changed: %s\n", path);
            return;
        }
    }

    fclose(fp);
    printf("File not found: %s\n", path);
}

// Function to add a password to a file
void add_password(const char *filesystem_name, const char *path, const char *password) {
    FILE *fp = fopen(filesystem_name, "rb+");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);

    for (uint32_t i = 0; i < fs.directory_count; i++) {
        if (strcmp((char*)fs.directory[i].name, path) == 0) {
            // Check if the entry is a directory
            if (fs.directory[i].size == 0) {
                fclose(fp);
                printf("Error: '%s' is a directory. Passwords can only be added to files.\n", path);
                return;
            }

            strncpy(fs.directory[i].password, password, sizeof(fs.directory[i].password) - 1);
            fseek(fp, 0, SEEK_SET);
            fwrite(&fs, sizeof(FileSystem), 1, fp);
            fclose(fp);
            printf("Password added: %s\n", path);
            return;
        }
    }

    fclose(fp);
    printf("File not found: %s\n", path);
}

// Function to dump the file system information
void dump_file_system(const char *filesystem_name) {
    FILE *fp = fopen(filesystem_name, "rb");
    if (!fp) {
        perror("Failed to open file system");
        return;
    }

    FileSystem fs;
    fread(&fs, sizeof(FileSystem), 1, fp);
    fclose(fp);

    uint32_t total_used_blocks = 0;

    // Print the initial table
    printf("\n%-30s | %-10s | %-20s | %-15s | %-10s | %-10s | %-25s\n", "File", "Size (KB)", "Used Block Count", "Password", "Read Perm", "Write Perm", "Creation Time");
    printf("----------------------------------------------------------------------------------------------------------------------------------------\n");

    for (uint32_t i = 0; i < fs.directory_count; i++) {
        DirectoryEntry entry = fs.directory[i];

        // Calculate the number of used blocks and check the FAT table
        uint16_t current_block = entry.first_block;
        uint32_t block_count = 0;

        // Add a check to indicate that directories do not have valid block numbers
        if (current_block == 0xFFFF) {
            printf("%-30s | %-10.1f | %-20u | %-15s | %-10c | %-10c | %-25s\n", 
                entry.name, entry.size / 1024.0, block_count, 
                entry.password[0] ? "Yes" : "No",
                (entry.permissions & PERMISSION_READ) ? 'Y' : 'N',
                (entry.permissions & PERMISSION_WRITE) ? 'Y' : 'N',
                ctime(&entry.creation_time));
            continue;
        }

        // Check if the block number is valid
        if (current_block >= FAT_ENTRIES && current_block != 0xFFFF) {
            printf("%-30s | %-10.1f | Error: Invalid block number %u\n", entry.name, entry.size / 1024.0, current_block);
            continue;
        }

        while (current_block != 0xFFF) {
            if (current_block >= FAT_ENTRIES) {
                printf("%-30s | %-10.1f | Error: Invalid block number %u\n", entry.name, entry.size / 1024.0, current_block);
                break;
            }
            block_count++;
            current_block = fs.fat[current_block];
        }
        printf("%-30s | %-10.1f | %-20u | %-15s | %-10c | %-10c | %-25s\n", 
            entry.name, entry.size / 1024.0, block_count, 
            entry.password[0] ? "Yes" : "No",
            (entry.permissions & PERMISSION_READ) ? 'Y' : 'N',
            (entry.permissions & PERMISSION_WRITE) ? 'Y' : 'N',
            ctime(&entry.creation_time));
        total_used_blocks += block_count;
    }

    printf("----------------------------------------------------------------------------------------------------------------------------------------\n");
    printf("Total Used Block Count: %u\n", total_used_blocks);

    // Print file system information
    printf("\nFile System Information:\n");
    printf("Total block count: %u\n", fs.total_blocks);
    printf("Free block count: %u\n", fs.free_blocks);
    printf("Directory count: %u\n", fs.directory_count);
    printf("Block size: %.1f KB\n", fs.block_size / 1024.0);
}
