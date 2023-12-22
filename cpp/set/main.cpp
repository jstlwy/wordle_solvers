#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

constexpr std::size_t WORDLE_WORD_LEN {5};

std::string get_arg_param(const std::vector<std::string>& args, std::string_view expected_arg)
{
    auto itr = std::find(args.begin(), args.end(), expected_arg);
    if (itr == args.end()) {
        return "";
    }
    itr++;
    if (itr == args.end()) {
        return "";
    }
    std::string param = *itr;
    if (param.at(0) != '-') {
        return param;
    }
    return "";
}

std::vector<std::string> split(const std::string& s, const char delim)
{
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;
    while (getline(ss, token, delim)) {
        result.push_back(token);
    }
    return result;
}

std::bitset<26> get_letters_from_param(const std::string& param)
{
    std::bitset<26> letter_set{0};
    std::vector<std::string> letters = split(param, ',');
    for (const std::string& letter : letters) {
        if (letter.length() != 1) {
            continue;
        }
        const char c = letter.at(0);
        if (std::isalpha(c)) {
            const std::size_t pos = c - 'a';
            letter_set.set(pos);
        }
    }
    return letter_set;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Error: No arguments were provided.\n";
        return EXIT_FAILURE;
    }

    // -----------------------------
    // PARSE COMMAND LINE ARGUMENTS
    // -----------------------------
    const std::vector<std::string> args(argv + 1, argv + argc);

    // Path to text file containing a list of words.
    const std::string wordFilePathParam = get_arg_param(args, "-list");
    const std::string wordFilePath = wordFilePathParam != "" ? wordFilePathParam : "../../wordlewords.txt";

    // List of letters known to not be in the word.
    // Separate multiple with a comma: -exclude m,s,e
    const std::string excludeArg = get_arg_param(args, "-exclude");

    // List of letters known to be in the word but whose positions are unknown.
    // Separate multiple with a comma: -require m,s,e
    const std::string requireArg = get_arg_param(args, "-require");

    // List of known positions and letters.
    // Separate multiple with a comma: -known 1m,2o,3u
    const std::string knownArg = get_arg_param(args, "-known");

    if (excludeArg.empty() && requireArg.empty() && knownArg.empty()) {
        std::cerr << "Error: No valid parameters were found for any of the options.\n";
        return EXIT_FAILURE;
    }

    // ----------------------------------
    // GET EXCLUDED AND REQUIRED LETTERS
    // ----------------------------------
    // Use sets to prevent any letters from appearing more than once
    const std::bitset<26> excludedLetterSet = get_letters_from_param(excludeArg);
    if (excludedLetterSet.count() >= 26) {
        std::cerr << "Error: All letters of the alphabet have been excluded.\n";
        return EXIT_FAILURE;
    }

    const std::bitset<26> requiredLetterSet = get_letters_from_param(requireArg);
    if (requiredLetterSet.count() > WORDLE_WORD_LEN) {
        std::cerr << "Error: More letters are required than are in the word.\n";
        return EXIT_FAILURE;
    }

    if ((excludedLetterSet & requiredLetterSet) != 0) {
        std::cerr << "Error: The set of excluded letters has one or more letters in common with the set of required letters.\n";
        return EXIT_FAILURE;
    }

    // --------------------
    // GET KNOWN POSITIONS
    // --------------------
    std::vector<std::string> knownArgs = split(knownArg, ',');
    std::array<char, WORDLE_WORD_LEN> knownPositions;
    knownPositions.fill('*');

    for (const std::string& arg : knownArgs) {
        if (arg.length() != 2) {
            continue;
        }
        const char position = arg.front();
        if (!std::isdigit(position) || (position < '1') || (position > '5')) {
            continue;
        }
        const char letter = arg.at(1);
        if (!std::isalpha(letter)) {
            continue;
        }
        const std::size_t i = (position - '0') - 1;
        knownPositions.at(i) = letter;
    }

    // ---------------------------------
    // APPLY ARGUMENTS TO WORDS IN FILE
    // ---------------------------------
    std::ifstream wordFile;
    wordFile.open(wordFilePath);
    if (!wordFile.is_open()) {
        std::cerr << "Error: Unable to open the list of words.\n";
        return EXIT_FAILURE;
    }

    std::string line;
    while (std::getline(wordFile, line)) {
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        if (line.length() != WORDLE_WORD_LEN) {
            continue;
        }

        bool is_valid_word = true;
        std::size_t numRequiredLetters = 0;
        for (std::size_t i = 0; i < WORDLE_WORD_LEN; i++) {
            const char currentLetter = line.at(i);
            if (!std::isalpha(currentLetter)) {
                is_valid_word = false;
                break;
            }
            const char knownLetter = knownPositions.at(i);
            if ((knownLetter != '*') && (currentLetter != knownLetter)) {
                is_valid_word = false;
                break;
            }
            const std::size_t currentPos = currentLetter - 'a';
            if (excludedLetterSet.test(currentPos)) {
                is_valid_word = false;
                break;
            }
            if (requiredLetterSet.test(currentPos)) {
                numRequiredLetters++;
            }
        }

        if (is_valid_word && (numRequiredLetters == requiredLetterSet.count())) {
            std::cout << line << "\n";
        }
    }

    wordFile.close();
    return EXIT_SUCCESS;
}

