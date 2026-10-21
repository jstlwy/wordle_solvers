// Wordle Solver

package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"strings"
	"unicode"
)

const wordleWordLen int = 5

func wordIsValidWordleWord(s string) bool {
	if len(s) != wordleWordLen {
		return false
	}

	doesContainOnlyLowercaseLetters := true
	for _, letter := range s {
		if !unicode.IsLower(letter) {
			doesContainOnlyLowercaseLetters = false
			break
		}
	}
	return doesContainOnlyLowercaseLetters
}

func main() {
	// -------------
	// SET UP FLAGS
	// -------------
	wordFilePath := flag.String(
		"dict",
		"../wordlewords.txt",
		"A path to a text file containing a list of words.")
	excludedArg := flag.String(
		"exclude",
		"",
		"A list of letters known to not be in the word. Example: -exclude mse")
	requiredArg := flag.String(
		"require",
		"",
		"A list of letters known to be in the word but whose positions are unknown. Example: -require mse")
	knownArg := flag.String(
		"known",
		"",
		"A list of known positions. Mark unknown positions with an underscore. Example: -known m__se")
	shouldSaveToTxtArg := flag.Bool(
		"save",
		false,
		"Save the potential solutions in a .txt file.")
	flag.Parse()

	// ------------------------
	// VALIDATE USER ARGUMENTS
	// ------------------------
	knownLetters := []rune(*knownArg)
	numKnownLetters := len(knownLetters)

	// It's pointless to run this program if no arguments were passed.
	if len(*excludedArg) == 0 && len(*requiredArg) == 0 && numKnownLetters == 0 {
		fmt.Fprintln(os.Stderr, "ERROR: No arguments were provided.")
		os.Exit(1)
	}

	someLettersAreKnown := false
	if numKnownLetters != 0 {
		if numKnownLetters != wordleWordLen {
			fmt.Fprintf(os.Stderr, "ERROR: The value of -known must contain exactly %v letters.\n", wordleWordLen)
			os.Exit(1)
		}
		for _, letter := range *knownArg {
			if !unicode.IsLower(letter) && letter != '_' {
				fmt.Fprintln(os.Stderr, "ERROR: The value of -known may only contain lowercase letters and underscores.")
				os.Exit(1)
			}
		}
		someLettersAreKnown = true
	}

	// ------------------
	// GET VALID LETTERS
	// ------------------
	// Use maps to prevent duplicate letters from appearing
	excludedLetterMap := make(map[rune]struct{})
	for _, letter := range strings.ToLower(*excludedArg) {
		if unicode.IsLower(letter) {
			excludedLetterMap[letter] = struct{}{}
		}
	}
	if len(excludedLetterMap) >= 25 {
		fmt.Fprintln(os.Stderr, "ERROR: Too many letters have been excluded.")
		os.Exit(1)
	}

	requiredLetterMap := make(map[rune]struct{})
	for _, letter := range strings.ToLower(*requiredArg) {
		if unicode.IsLower(letter) {
			requiredLetterMap[letter] = struct{}{}
		}
	}
	if len(requiredLetterMap) > wordleWordLen {
		fmt.Fprintln(os.Stderr, "ERROR: There are more required letters than there are letters in the word.")
		os.Exit(1)
	}

	for letter := range requiredLetterMap {
		if _, ok := excludedLetterMap[letter]; ok {
			fmt.Fprintln(os.Stderr, "ERROR: The set of required letters and the set of excluded letters are not disjoint.")
			os.Exit(1)
		}
	}

	// ---------------------------------
	// APPLY ARGUMENTS TO WORDS IN FILE
	// ---------------------------------
	// Read in words from text file
	txtFile, err := os.Open(*wordFilePath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "ERROR: Failed to open \"%s\": %v\n", *wordFilePath, err)
		os.Exit(1)
	}
	defer txtFile.Close()

	var wordList []string
	requiredLettersFound := make(map[rune]struct{}, wordleWordLen)

	scanner := bufio.NewScanner(txtFile)
	for scanner.Scan() {
		word := strings.ToLower(scanner.Text())
		if !wordIsValidWordleWord(word) {
			continue
		}

		isPossibleSolution := true
		clear(requiredLettersFound)

		for i, letter := range word {
			// First, check against the known letters
			if someLettersAreKnown {
				knownLetter := knownLetters[i]
				if knownLetter != '_' && letter != knownLetter {
					isPossibleSolution = false
					break
				}
			}

			// Check against the excluded letters
			if _, ok := excludedLetterMap[letter]; ok {
				isPossibleSolution = false
				break
			}

			// Finally, keep track of how many required letters were found
			if _, ok := requiredLetterMap[letter]; ok {
				requiredLettersFound[letter] = struct{}{}
			}
		}

		if isPossibleSolution && len(requiredLettersFound) == len(requiredLetterMap) {
			wordList = append(wordList, word)
		}
	}

	if err = scanner.Err(); err != nil {
		fmt.Fprintf(os.Stderr, "ERROR: Scanner failed: %v\n", err)
		os.Exit(1)
	}

	// -------------
	// SHOW RESULTS
	// -------------
	fmt.Printf("%d possible solutions:\n", len(wordList))
	for _, word := range wordList {
		fmt.Println(word)
	}

	if *shouldSaveToTxtArg {
		txtFilename := "results.txt"
		if _, err = os.Stat(txtFilename); err == nil {
			err = os.Remove(txtFilename)
			if err != nil {
				fmt.Fprintf(os.Stderr, "ERROR: Failed to remove existing file \"%s\": %v\n", txtFilename, err)
				os.Exit(1)
			}
		}

		txtFile, err := os.OpenFile(txtFilename, os.O_WRONLY|os.O_CREATE, 0644)
		if err != nil {
			fmt.Fprintf(os.Stderr, "ERROR: Failed to open \"%s\": %v\n", txtFilename, err)
			os.Exit(1)
		}
		defer txtFile.Close()

		txtFile.WriteString("Potential solutions:\n")
		for _, word := range wordList {
			txtFile.WriteString(word + "\n")
		}
	}
}
