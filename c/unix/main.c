#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#define NUM_LETTERS_IN_ALPHABET 26
#define WORDLE_WORD_LEN 5
#define LINE_BUFFER_LEN 8

int get_param_index(const int argc, char* argv[const argc], const char expected_arg[]);
void print_letters_in_mask(uint32_t mask);
int get_letters_from_param(const int i_param, char* argv[], const size_t max_num_letters, uint32_t* mask);

int main(int argc, char* argv[])
{
    uint32_t exclude_mask = 0;
    const int i_exclude = get_param_index(argc, argv, "-exclude");
    if (i_exclude > 1) {
        if (get_letters_from_param(i_exclude, argv, NUM_LETTERS_IN_ALPHABET - 1, &exclude_mask) < 0) {
            fputs("ERROR: Invalid list of letters to exclude.\n", stderr);
            return EXIT_FAILURE;
        }
        puts("Excluded letters:");
        printf("0x%08" PRIx32 "\n", exclude_mask);
        print_letters_in_mask(exclude_mask);
        putchar('\n');
    }

    uint32_t require_mask = 0;
    const int i_require = get_param_index(argc, argv, "-require");
    if (i_require > 1) {
        if (get_letters_from_param(i_require, argv, WORDLE_WORD_LEN, &require_mask) < 0) {
            fputs("ERROR: Invalid list of letters to require.\n", stderr);
            return EXIT_FAILURE;
        }
        puts("Required letters:");
        printf("0x%08" PRIx32 "\n", require_mask);
        print_letters_in_mask(require_mask);
        putchar('\n');
    }

    if (exclude_mask & require_mask) {
        fputs("ERROR: The set of excluded letters and the set of required letters are not disjoint.\n", stderr);
        return EXIT_FAILURE;
    }

    size_t num_known_letters = 0;
    char known_letters[WORDLE_WORD_LEN];
    memset(known_letters, '_', WORDLE_WORD_LEN);
    const int i_known = get_param_index(argc, argv, "-known");
    if (i_known > 1) {
        const char* known_param = argv[i_known];
        const size_t known_param_len = strlen(known_param);
        if (known_param_len != WORDLE_WORD_LEN) {
            fprintf(stderr, "ERROR: The known letters argument must contain exactly %d characters.\n", WORDLE_WORD_LEN);
            return EXIT_FAILURE;
        }

        memcpy(known_letters, known_param, WORDLE_WORD_LEN);
        for (size_t i = 0; i < WORDLE_WORD_LEN; ++i) {
            const char c = known_letters[i];
            const bool is_letter = isalpha(c);
            if (!is_letter && c != '_') {
                fputs("ERROR: The known letters argument may only contain letters or underscores.\n", stderr);
                return EXIT_FAILURE;
            }
            if (is_letter && isupper(c)) {
                // Convert to lowercase
                static const char CASE_DISTANCE = 'a' - 'A';
                known_letters[i] -= CASE_DISTANCE;
                ++num_known_letters;
            }
        }

        if (num_known_letters > 0) {
            printf("Known letters:\n%.*s\n\n", WORDLE_WORD_LEN, known_letters);
        }
    }

    static const char* const word_file_path = "../wordlewords.txt";
    const int word_file_fd = open(word_file_path, O_RDONLY);
    if (word_file_fd < 0) {
        fprintf(stderr, "ERROR: Failed to open \"%s\": %s\n", word_file_path, strerror((errno)));
        return EXIT_FAILURE;
    }

    struct stat statbuf;
    if (fstat(word_file_fd, &statbuf) < 0) {
        fprintf(stderr, "ERROR: Failed to get file information for \"%s\": %s\n", word_file_path, strerror(errno));
        return EXIT_FAILURE;
    }

    const size_t file_size = (size_t)statbuf.st_size;
    char* word_file = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, word_file_fd, 0);
    if (word_file == MAP_FAILED) {
        fprintf(stderr, "ERROR: Failed to map \"%s\": %s\n", word_file_path, strerror((errno)));
        return EXIT_FAILURE;
    }

    close(word_file_fd);

    size_t i_file = 0;
    char word_buffer[WORDLE_WORD_LEN + 1];
    word_buffer[WORDLE_WORD_LEN] = '\n';

    while (i_file < file_size) {
        // Find the start of the next word
        while (!isalpha(word_file[i_file]) && (i_file < file_size)) {
            ++i_file;
        }
        if (i_file >= file_size) {
            break;
        }
        const size_t i_start = i_file;

        // Find the end of the word
        while (!isspace(word_file[i_file]) && (i_file < file_size)) {
            ++i_file;
        }
        if (i_file >= file_size) {
            break;
        }
        const size_t i_end = i_file;

        const size_t word_size = i_end - i_start;
        if (word_size != WORDLE_WORD_LEN) {
            continue;
        }

        memcpy(word_buffer, word_file + i_start, WORDLE_WORD_LEN);
        bool is_valid_word = true;
        uint32_t found_letters = 0;

        for (size_t i = 0; i < WORDLE_WORD_LEN; ++i) {
            const char current_letter = word_buffer[i];
            if (!isalpha(current_letter)) {
                is_valid_word = false;
                break;
            }

            const char known_letter = known_letters[i];
            if ((known_letter != '_') && (current_letter != known_letter)) {
                is_valid_word = false;
                break;
            }

            const size_t pos = current_letter - 'a';
            const uint32_t mask = 1 << pos;
            if (exclude_mask & mask) {
                is_valid_word = false;
                break;
            }
            found_letters |= mask;
        }

        if (is_valid_word && ((found_letters & require_mask) == require_mask)) {
            write(STDOUT_FILENO, word_buffer, WORDLE_WORD_LEN + 1);
        }
    }

    munmap(word_file, file_size);
    return EXIT_SUCCESS;
}

int get_param_index(const int argc, char* argv[const argc], const char expected_arg[])
{
    // First, try to find the expected argument
    int i_arg = -1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(expected_arg, argv[i]) == 0) {
            i_arg = i;
            break;
        }
    }

    // Check whether the argument was found
    // and whether a parameter might even exist
    const int i_param = i_arg + 1;
    if ((i_arg < 0) || (i_param >= argc)) {
        return -1;
    }
    return i_param;
}

size_t get_population_count(uint32_t num)
{
    size_t count = 0;
    while (num != 0) {
        ++count;
        num &= num - 1;
    }
    return count;
}

int get_letters_from_param(
    const int i_param,
    char* argv[],
    const size_t max_num_letters,
    uint32_t* mask
) {
    if (i_param <= 1) {
        fputs("ERROR: Invalid parameter index.\n", stderr);
        return -1;
    }

    const char* param = argv[i_param];
    const size_t param_len = strlen(param);

    for (size_t i = 0; i < param_len; ++i) {
        const char c = param[i];
        if (!isalpha(c)) {
            fputs("ERROR: Non-alphabetic character found in the list.\n", stderr);
            return -1;
        }
        const char first_letter = islower(c) ? 'a' : 'A';
        const int letter_num = c - first_letter;
        *mask |= 1 << letter_num;
    }

    if (get_population_count(*mask) > max_num_letters) {
        fprintf(stderr, "ERROR: The list must contain less than %zu words.\n", max_num_letters);
        return -1;
    }

    return 0;
}

void print_letters_in_mask(const uint32_t mask)
{
    char alphabet[NUM_LETTERS_IN_ALPHABET];
    int num_letters_in_mask = 0;
    for (int i = 0; i < NUM_LETTERS_IN_ALPHABET; ++i) {
        const uint32_t check_bitmask = 1 << i;
        if (mask & check_bitmask) {
            alphabet[num_letters_in_mask] = 'a' + (char)i;
            ++num_letters_in_mask;
        }
    }
    if (num_letters_in_mask) {
        printf("%.*s\n", num_letters_in_mask, alphabet);
    } else {
        puts("(none)");
    }
}
