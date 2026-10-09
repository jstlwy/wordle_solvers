// Web sites that proved useful:
// https://learnxinyminutes.com/odin/

package main

import "core:flags"
import "core:fmt"
import "core:os"
import "core:strings"

NUM_LETTERS_IN_ALPHABET :: 26
WORDLE_WORD_LEN :: 5

is_alpha_rune :: proc(c: rune) -> bool {
    return (c >= 'a') && (c <= 'z')
}

main :: proc() {
    Options :: struct {
        exclude: string `usage:"Letters that are not in the word."`,
        require: string `usage:"Letters that are somewhere in the word."`,
        known  : string `usage:"Letters whose positions are known. Use \"_\" for unknowns."`,
    }
    opt: Options
    flags.parse_or_exit(&opt, os.args, flags.Parsing_Style.Unix)

    excluded_letters_map := make(map[rune]bool)
    defer delete(excluded_letters_map)
    if len(opt.exclude) >= NUM_LETTERS_IN_ALPHABET {
        fmt.eprintln("ERROR: Too many letters to exclude.")
        os.exit(1)
    }
    for letter in opt.exclude {
        if !is_alpha_rune(letter) {
            fmt.eprintln("ERROR: The exclude list contains an invalid character.")
            os.exit(1)
        }
        excluded_letters_map[letter] = true
    }
    fmt.println("Map of excluded letters:")
    fmt.println(excluded_letters_map)

    required_letters_map := make(map[rune]bool)
    defer delete(required_letters_map)
    if len(opt.require) > WORDLE_WORD_LEN {
        fmt.eprintln("ERROR: Too many letters to require.")
        os.exit(1)
    }
    for letter, index in opt.require {
        if !is_alpha_rune(letter) {
            fmt.eprintln("ERROR: The require list contains an invalid character.")
            os.exit(1)
        }
        required_letters_map[letter] = false
    }
    fmt.println("Map of required letters:")
    fmt.println(required_letters_map)
    num_required_letters := len(required_letters_map)

    known_positions_array: [WORDLE_WORD_LEN]rune
    if opt.known == "" {
        for i in 0..<len(known_positions_array) {
            known_positions_array[i] = '_'
        }
    } else {
        if len(opt.known) != WORDLE_WORD_LEN {
            fmt.eprintfln("ERROR: The known positions string must contain exactly %d characters.", WORDLE_WORD_LEN)
            os.exit(1)
        }
        for letter, index in opt.known {
            if letter != '_' && !is_alpha_rune(letter) {
                fmt.eprintln("ERROR: The known list contains an invalid character.")
                os.exit(1)
            }
            known_positions_array[index] = letter
        }
    }
    fmt.println("Map of known positions:")
    fmt.println(known_positions_array)

    filepath :: "../wordlewords.txt"
    words, err := os.read_entire_file(filepath, context.allocator)
    if err != nil {
        fmt.eprintfln("ERROR: Failed to open %s.", filepath)
        os.exit(1)
    }
    defer delete(words, context.allocator)

    unique_found_letters_map := make(map[rune]bool)
    defer delete(unique_found_letters_map)

    // https://odin-lang.org/news/read-a-file-line-by-line/
    it := string(words)
    for word in strings.split_lines_iterator(&it) {
        word_len := len(word)
        if word_len != WORDLE_WORD_LEN {
            fmt.eprintfln("ERROR: Encountered a word that contained %d characters: %s", word_len, word)
            os.exit(1)
        }

        is_valid_word := true

        for letter, index in word {
            if !is_alpha_rune(letter) {
                is_valid_word = false
                break
            }

            if letter in excluded_letters_map {
                is_valid_word = false
                break
            }

            known_letter := known_positions_array[index]
            if known_letter != '_' && known_letter != letter {
                is_valid_word = false
                break
            }

            if letter in required_letters_map {
                unique_found_letters_map[letter] = true
            }
        }

        if is_valid_word && len(unique_found_letters_map) == num_required_letters {
            fmt.println(word)
        }

        clear(&unique_found_letters_map)
    }
}
