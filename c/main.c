#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#define WORDLE_WORD_LEN 5
#define LINE_BUFFER_LEN 8

void print_letters_in_mask(uint32_t mask);
size_t get_population_count(uint32_t num);
bool is_alpha_char(const char c);
bool is_digit_char(const char c);
uint32_t alpha_to_mask(const char c);

int get_param_index(const int argc, char* argv[argc], const char expected_arg[]);
uint32_t get_letters_from_param(const int i_param, char* argv[]);
void get_known_char(char known_letters[const static WORDLE_WORD_LEN], const char param[const]);

int main(int argc, char* argv[])
{
    if (argc < 3) {
        fputs("Error: Not enough arguments were provided.\n", stderr);
        return EXIT_FAILURE;
    }

    const int i_exclude = get_param_index(argc, argv, "-exclude");
    const uint32_t excluded_letters = get_letters_from_param(i_exclude, argv);
    puts("Excluded letters:");
    print_letters_in_mask(excluded_letters);
    puts("");
    if (get_population_count(excluded_letters) >= 26) {
        fputs("Error: All letters of the alphabet have been excluded.\n", stderr);
        return EXIT_FAILURE;
    }

    const int i_require = get_param_index(argc, argv, "-require");
    const uint32_t required_letters = get_letters_from_param(i_require, argv);
    puts("Required letters:");
    print_letters_in_mask(required_letters);
    puts("");
    if (get_population_count(required_letters) > WORDLE_WORD_LEN) {
        fputs("Error: More letters are required than are in the word.\n", stderr);
        return EXIT_FAILURE;
    }

    if (excluded_letters & required_letters) {
        fputs("Error: The set of excluded letters and the set of required letters are not disjoint.\n", stderr);
        return EXIT_FAILURE;
    }

    char known_letters[5] = "_____";
    const int i_known = get_param_index(argc, argv, "-known");
    if (i_known > 1) {
        const char* known_param = argv[i_known];
        size_t num_known = 0;

        // While there are commas (2 or more letters)
        const char* p_comma = strchr(known_param, ',');
        while (p_comma != NULL) {
            if (num_known > 4) {
                fputs("Error: Too many known letters.\n", stderr);
                return EXIT_FAILURE;
            }
            if ((p_comma - known_param) != 2) { 
                fputs("Error: Invalid argument format for known letters.\n", stderr);
                return EXIT_FAILURE;
            }
            get_known_char(known_letters, known_param);
            num_known++;
            known_param = p_comma + 1;
            p_comma = strchr(known_param, ',');
        }

        // Check for the last letter (or if there was only ever 1)
        if (num_known > 4) {
            fputs("Error: Too many known letters.\n", stderr);
            return EXIT_FAILURE;
        }
        if (strlen(known_param) != 2) {
            fputs("Error: Invalid argument format for known letters.\n", stderr);
            return EXIT_FAILURE;
        }
        get_known_char(known_letters, known_param);
    }
    printf("Known letters:\n[%*s]\n\n", WORDLE_WORD_LEN, known_letters);

    FILE* const wordfile = fopen("../wordlewords.txt", "r");
    if (wordfile == NULL) {
        fprintf(stderr, "Error: Unable to open the list of words: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    char line_buffer[LINE_BUFFER_LEN];
    while (fgets(line_buffer, LINE_BUFFER_LEN, wordfile) != NULL) {
        const char* const p_space = strchr(line_buffer, '\n');
        if ((p_space - line_buffer) != WORDLE_WORD_LEN) {
            continue;
        }

        bool is_valid_word = true;
        uint32_t found_letters = 0;

        for (size_t i = 0; i < WORDLE_WORD_LEN; i++) {
            const char current_letter = line_buffer[i];
            if (!is_alpha_char(current_letter)) {
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
            if (excluded_letters & mask) {
                is_valid_word = false;
                break;
            }
            found_letters |= mask;
        }

        if (is_valid_word && ((found_letters & required_letters) == required_letters)) {
            fputs(line_buffer, stdout);
        }
    }

    fclose(wordfile);
    return EXIT_SUCCESS;
}

void print_letters_in_mask(uint32_t mask)
{
    char alphabet[27];
    for (int i = 0; i < 26; i++, mask = mask >> 1) {
        if (mask & 0x00000001) {
            alphabet[i] = 'a' + i;
        } else {
            alphabet[i] = ' ';
        }
    }
    alphabet[26] = '\0';
    printf("[%s]\n", alphabet);
}

size_t get_population_count(uint32_t num)
{
    size_t count = 0;
    while (num != 0) {
        count++;
        num &= num - 1;
    }
    return count;
}

inline bool is_alpha_char(const char c)
{
    return (c >= 'a') && (c <= 'z');
}

inline bool is_digit_char(const char c)
{
    return (c >= '0') && (c <= '9');
}

uint32_t alpha_to_mask(const char c)
{
    if (!is_alpha_char(c)) {
        fputs("Error: Encountered a character that is not a letter.\n", stderr);
        exit(EXIT_FAILURE);
    }
    const size_t pos = c - 'a';
    const uint32_t mask = 1 << pos;
    return mask;
}

int get_param_index(const int argc, char* argv[argc], const char expected_arg[])
{
    // First, try to find the expected argument
    int i_arg = -1;
    for (int i = 1; i < argc; i++) {
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

uint32_t get_letters_from_param(const int i_param, char* argv[])
{
    if (i_param <= 1) {
        return 0;
    }

    uint32_t letter_set = 0;
    const char* param = argv[i_param];

    // While there are commas (2 or more letters)
    const char* p_comma = strchr(param, ',');
    while (p_comma != NULL) {
        if ((p_comma - param) != 1) {
            fputs("Error: Invalid argument format for excluded or required letters.\n", stderr);
            exit(EXIT_FAILURE);
        }
        letter_set |= alpha_to_mask(param[0]);
        param = p_comma + 1;
        p_comma = strchr(param, ',');
    }

    // Check for the last letter (or if there was only ever 1)
    if (strlen(param) != 1) {
        fputs("Error: Invalid argument format for excluded or required letters.\n", stderr);
        exit(EXIT_FAILURE);
    }
    letter_set |= alpha_to_mask(param[0]);

    return letter_set;
}

void get_known_char(char known_letters[const static 5], const char param[const])
{
    if (!is_digit_char(param[0]) || !is_alpha_char(param[1])) {
        fputs("Error: Format for known characters must be [index][character], e.g. 1e\n", stderr);
        exit(EXIT_FAILURE);
    }
    size_t i = param[0] - '0';
    if ((i < 1) || (i > WORDLE_WORD_LEN)) {
        fprintf(stderr, "Error: Invalid character index (%zu). Valid options are [1, %d].\n", i, WORDLE_WORD_LEN);
        exit(EXIT_FAILURE);
    }
    known_letters[i-1] = param[1];
}
