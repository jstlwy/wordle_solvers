// Wordle Solver

package main

import (
    "bufio"
    "flag"
    "fmt"
    "os"
    "regexp"
    "strconv"
    "strings"
)

func filterWordsWithoutIncludedLetters(wordList []string, wordLength int, includeArg string) []string {
    if len(wordList) == 0 || wordLength < 2 || len(includeArg) == 0 {
        return wordList	
    }

    includedLetters := make(map[string]bool)
    includeArgs := strings.Split(strings.ToLower(includeArg), ",")
    for _, arg := range includeArgs {
        if len(arg) != 1 {
            continue
        }
        matched, err := regexp.MatchString(`[a-z]`, arg)
        if err == nil && matched {
            includedLetters[arg] = true
        }
    }

    if len(includedLetters) == 0 || len(includedLetters) > wordLength {
        return wordList
    }

    var filteredWords []string
    for _, word := range wordList {
        wordIsValid := true
        for letter, _ := range includedLetters {
            if !strings.Contains(word, letter) {
                wordIsValid = false
                break
            }
        }
        if wordIsValid {
            filteredWords = append(filteredWords, word)
        }
    }

    if len(filteredWords) < len(wordList) {
        return filteredWords
    }
    return wordList
}


func main() {
    // -------------
    // SET UP FLAGS
    // -------------
    verbose := flag.Bool(
        "verbose",
        false,
        "Show how the user's arguments were interpreted.")
    wordFilePath := flag.String(
        "dict",
        "../wordlewords.txt",
        "Path to text file containing a list of words.")
    wordLength := flag.Int(
        "length",
        5,
        "The length of the word to be found.")
    excludeArg := flag.String(
        "exclude",
        "",
        "List of letters known to not be in the word. " +
        "Separate multiple with a comma: -exclude m,s,e")
    includeArg := flag.String(
        "include",
        "",
        "List of letters known to be in the word but whose positions are unknown. " +
        "Separate multiple with a comma: -include m,s,e")
    knownArg := flag.String(
        "known",
        "",
        "List of known positions and letters. Separate multiple with a comma: -known 1m,2o,3u")
    saveToTxt := flag.Bool(
        "save",
        false,
        "Save the potential solutions in a .txt file.")
        flag.Parse()

    // ------------------------
    // VALIDATE USER ARGUMENTS
    // ------------------------
    if *wordLength < 2 {
        fmt.Printf("Error: Word length must be at least 2.\n")
        os.Exit(1)
    }
    if len(*excludeArg) == 0 && len(*includeArg) == 0 && len(*knownArg) == 0 {
        fmt.Println("Error: No arguments were provided.")
        os.Exit(1)
    }

    // ------------------
    // GET VALID LETTERS
    // ------------------
    // Use map to prevent duplicate letters from appearing
    excludedLetterMap := make(map[string]bool)
    excludeArgs := strings.Split(strings.ToLower(*excludeArg), ",")
    for _, arg := range excludeArgs {
        if len(arg) != 1 {
            continue
        }
        matched, err := regexp.MatchString(`[a-z]`, arg)
        if err == nil && matched {
            excludedLetterMap[arg] = true
        }
    }

    // Now use the map to create a string of valid letters
    letterGroup := ""
    for i := 0; i < 26; i++ {
        letterRune := rune('a' + i)
        letterStr := string(letterRune)
        if _, ok := excludedLetterMap[letterStr]; ok {
            letterGroup += letterStr
        }
    }
    if len(letterGroup) == 26 {
        fmt.Println("Error: All 26 letters of the alphabet have been excluded.")
        os.Exit(1)
    } else if len(letterGroup) == 0 {
        letterGroup = "[a-z]"
    } else {
        letterGroup = "[^" + letterGroup + "]"
    }
    if *verbose {
        fmt.Printf("Regex letter group for unknown positions:\n%s\n\n", letterGroup)
    }

    // --------------------
    // GET KNOWN POSITIONS
    // --------------------
    // Use map to prevent duplicate values from appearing
    knownPositionsMap := make(map[int]string)
    knownPositionsRegex := regexp.MustCompile(`(\d+)([a-z])`)
    knownArgs := strings.Split(strings.ToLower(*knownArg), ",")
    for _, arg := range knownArgs {
        matches := knownPositionsRegex.FindStringSubmatch(arg)
        if matches == nil || len(matches) < 3 {
            continue
        }
        position, err := strconv.Atoi(matches[1])
        if err != nil || position < 1 || position > *wordLength {
            continue
        }
        knownPositionsMap[position - 1] = matches[2]
    }

    // --------------------
    // BUILD REGEX PATTERN
    // --------------------
    regexString := ""
    if len(knownPositionsMap) == 0 {
        regexString = letterGroup + "{" + strconv.Itoa(*wordLength) + "}"
    } else {
        for i := 0; i < *wordLength; i++ {
            if letter, ok := knownPositionsMap[i]; ok {
                regexString += letter
            } else {
                regexString += letterGroup
            }
        }
    }
    regexString = "^" + regexString + "$"
    if *verbose {
        fmt.Printf("Regex pattern to apply to each word:\n%s\n\n", regexString)
    }
    wordleRegex := regexp.MustCompile(regexString)

    // ---------------------------------
    // APPLY ARGUMENTS TO WORDS IN FILE
    // ---------------------------------
    // Read in words from text file
    txtFile, err := os.Open(*wordFilePath)
    if err != nil {
        fmt.Printf("Error when opening \"%s\": %v\n", *wordFilePath, err)
        os.Exit(1)
    }
    var wordList []string
    scanner := bufio.NewScanner(txtFile)
    for scanner.Scan() {
        word := strings.ToLower(scanner.Text())
        if len(word) == *wordLength && wordleRegex.MatchString(word) {
            wordList = append(wordList, word)
        }
    }
    if err = scanner.Err(); err != nil {
        fmt.Printf("Error when creating new Scanner: %v\n", err)
        os.Exit(1)
    }
    txtFile.Close()

    wordList = filterWordsWithoutIncludedLetters(
        wordList,
        *wordLength,
        *includeArg,
    )

    // -------------
    // SHOW RESULTS
    // -------------
    fmt.Printf("%d possible solutions:\n", len(wordList))
    for _, word := range wordList {
        fmt.Println(word)
    }

    if *saveToTxt {
        txtFilename := "results.txt"
        if _, err = os.Stat(txtFilename); err == nil {
            err = os.Remove(txtFilename)
            if err != nil {
                fmt.Printf("Error when attempting to remove existing file \"%s\": %v\n", txtFilename, err)
                os.Exit(1)
            }
        }
        txtFile, err := os.OpenFile(txtFilename, os.O_WRONLY|os.O_CREATE, 0644)
        if err != nil {
            fmt.Printf("Error when opening \"%s\": %v\n", txtFilename, err)
            os.Exit(1)
        }

        txtFile.WriteString("Potential solutions:\n")
        for _, word := range wordList {
            txtFile.WriteString(word + "\n")
        }

        txtFile.Close()
    }
}
