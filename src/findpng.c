/**

    Purpose:
    - recursively seraches through a directory and its subdirectories
    to find valid PNG fils based on their signature. 
    It prints the relative path of each valid PNG found.
    
    The PNG file format beings with a fixed 8-byte signature, and this is used to check whether a file is a valid PNG.
    The logic avoids symbolic links, handles path construction carefully to avoid buffer overflows, and counts how many PNG files are discovered.
    
     */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include "lab_png.h" // Provides the is_png function and type aliases like U8

#define PNG_SIG_SIZE 8 // Standard PNG signature length
#define PATH_MAX_LEN 4096 // Maximum allowed path length

/** 
    Recursively searches the given base directory and relative path for PNG files
    
    - base_path: absolute or root path to start search
    - relative_path: subdirectory or current traversal point
    - png_count: pointer to counter for number of PNGs found 
    - Buffer : a temporary array in memory used to hold data - specifically, character stings representing file paths or byte sequences from a file*/

void search_directory(const char *base_path, const char *relative_path, int *png_count) {
    char full_path[PATH_MAX_LEN]; // holds full resolved path to open directory
    char new_relative_path[PATH_MAX_LEN]; // holds next-level relative path for recursion
    DIR *dir;
    struct dirent *entry;
    struct stat sb;

    // Build full path 
    // construct the directory path to open
    if (strcmp(relative_path, ".") == 0) {
        snprintf(full_path, sizeof(full_path), "%s", base_path);

    // Safely format and write a string to a buffer
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, relative_path);
    // Safely format and write a string to a buffer
    }

    dir = opendir(full_path);
    if (!dir) {
    // print an error or message to stederr
        fprintf(stderr, "Error: Cannot open directory '%s'\n", full_path);
        return;
    }

    while ((entry = readdir(dir))) {
        // Skip . and ..
        // skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Check path length before constructing paths
        // check if new full path or relative path will exceed buffer size
        size_t full_path_len = strlen(full_path);
        size_t entry_name_len = strlen(entry->d_name);
        size_t relative_path_len = strlen(relative_path);
        
        // Check if new_full_path would exceed buffer
        // avoid path overflow
        if (full_path_len + 1 + entry_name_len >= PATH_MAX_LEN) {
            fprintf(stderr, "Warning: Path too long for '%s/%s'\n", full_path, entry->d_name);
            // print an error or message to stderr
            continue;
        }
        
        // Check if new_relative_path would exceed buffer
        size_t new_relative_len = (strcmp(relative_path, ".") == 0) ? 
            entry_name_len : relative_path_len + 1 + entry_name_len;
        if (new_relative_len >= PATH_MAX_LEN) {
            fprintf(stderr, "Warning: Relative path too long\n"); // print an error or message to stderr
            continue;
        }

        // Construct new relative path
        // construct new relative path to pass for recursion or output
        if (strcmp(relative_path, ".") == 0) {
            snprintf(new_relative_path, sizeof(new_relative_path), "%s", entry->d_name);// Safely format and write a string to a buffer
        } else {
            snprintf(new_relative_path, sizeof(new_relative_path), "%s/%s", relative_path, entry->d_name);// Safely format and write a string to a buffer
        }

        // Construct full path for stat
        // build full path for the file to use in stat
        char new_full_path[PATH_MAX_LEN];
        snprintf(new_full_path, sizeof(new_full_path), "%s/%s", full_path, entry->d_name);  // safely format and write a string to a buffer

        if (lstat(new_full_path, &sb) == -1) { // get file information
            fprintf(stderr, "Warning: Cannot stat '%s'\n", new_full_path); 
            continue;
        }


        // skip symbolic links
        if (S_ISLNK(sb.st_mode)) {
            continue; // Skip symbolic links
        } else if (S_ISDIR(sb.st_mode)) {
        // recurse into subdirectory
            search_directory(base_path, new_relative_path, png_count); // Recurse
        } else if (S_ISREG(sb.st_mode)) {
        // handle regular file: check if it's a PNG
            FILE *fp = fopen(new_full_path, "rb"); // open a file stream
            if (!fp) {
                fprintf(stderr, "Warning: Cannot open file '%s'\n", new_full_path); // print an error or message to stderr
                continue;
            }

            U8 sig[PNG_SIG_SIZE]; // Buffer to hold signature bytes
            size_t nread = fread(sig, 1, PNG_SIG_SIZE, fp); // Read data from file
            fclose(fp);

            if (nread == PNG_SIG_SIZE && is_png(sig, PNG_SIG_SIZE)) {
            // clean up "./" prefix if present
                char absolute_path[PATH_MAX_LEN];
                if (realpath(new_full_path, absolute_path)) {
                    printf("%s\n", absolute_path);
                    (*png_count)++;
                }

                (*png_count)++;
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) { // Main entry point of the program
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Validate input directory
    struct stat sb;
    if (stat(argv[1], &sb) == -1 || !S_ISDIR(sb.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a valid directory\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Start recursive PNG search
    int png_count = 0;
    search_directory(argv[1], ".", &png_count);

    // final report
    if (png_count == 0) {
        printf("findpng: No PNG file found\n");
    }

    return EXIT_SUCCESS;
}