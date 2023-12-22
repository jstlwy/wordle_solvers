using System.IO;
using System.CommandLine;
using System.Text.RegularExpressions;

static class Program
{
    public record WordCollection(string fileName, int expectedCount);

    const string defaultWordFileName = "../wordlewords.txt";

    static string[] GetAllWordsFromFile(string fileName, int expectedWordCount)
    {
        if (!File.Exists(fileName))
        {
            Console.Error.WriteLine($"Fatal error: {fileName} not found. Terminating program.");
            Environment.Exit(-1);
        }

        string[] words = File.ReadAllLines(fileName);
        if (words.Count() != expectedWordCount)
        {
            Console.Error.WriteLine($"Warning: Expected {expectedWordCount} words in {fileName} but only found {words.Count()}.");
        }

        return words;
    }

    static string[] GetAllUniqueWordsFromCollections(IEnumerable<WordCollection> wordCollections)
    {
        HashSet<string> allUniqueWords = new HashSet<string>();
        foreach (WordCollection wc in wordCollections)
        {
            string[] words = GetAllWordsFromFile(wc.fileName, wc.expectedCount);
            HashSet<string> wordSet = words
                .Where(word => word.Length == 5)
                .Select(word => word.ToLower())
                .ToHashSet();
            allUniqueWords.UnionWith(wordSet);
        }
        return allUniqueWords.Order().ToArray();
    }

    static void SolveWordle(string wordFileName, int wordLength, string regexString,
            List<char> requiredLetters, bool shouldSaveToTxt)
    {
        if (!File.Exists(wordFileName))
        {
            Console.WriteLine("Fatal error: Word file not found. Terminating program.");
            return;
        }

        // Read in all the words from the file and filter accordingly
        Regex wordleRegex = new Regex(regexString);
        List<string> wordList = File.ReadAllLines(wordFileName)
            .Select(line => line.ToLower())
            .Where(line => line.Length == wordLength && wordleRegex.IsMatch(line))
            .ToList();

        // Exclude words that are missing any required letters
        if (requiredLetters.Any())
        {
            wordList.RemoveAll(word => requiredLetters.Except(word).Any());
        }

        if (shouldSaveToTxt)
        {
            File.WriteAllLines("solutions.txt", wordList);
            return;
        }

        Console.WriteLine($"{wordList.Count} possible solutions:");
        foreach (string word in wordList)
        {
            Console.WriteLine(word);
        }
    }

    static void Main(string[] args)
    {
        var verboseOption = new Option<bool>(
                name: "--verbose",
                description: "Show how the user's parameters were transformed into a regular expression.",
                getDefaultValue: () => false
                );
        var wordFileOption = new Option<string>(
                name: "-dict",
                description: "User-specified text file from which to read in English-language words.",
                getDefaultValue: () => defaultWordFileName
                );
        var wordLengthOption = new Option<int>(
                name: "-length",
                description: "The length of the word to be found.",
                getDefaultValue: () => 5
                );
        var excludeOption = new Option<string>(
                name: "-exclude",
                description: "List of letters that are known to not be in the word. " +
                "Separate multiple with a comma. " +
                "For example: -exclude m,s,e",
                getDefaultValue: () => ""
                );
        var includeOption = new Option<string>(
                name: "-include",
                description: "List of letters that are known to be in the word but whose " +
                "positions are unknown. Separate multiple with a comma. " +
                "For example: -include m,s,e",
                getDefaultValue: () => ""
                );
        var knownOption = new Option<string>(
                name: "-known",
                description: "List of known positions and letters. " +
                "Separate multiple with a comma. " +
                "For example: -known 1m,2o,3u",
                getDefaultValue: () => ""
                );
        var saveToTxtOption = new Option<bool>(
                name: "--save",
                description: "Save the potential solutions in a .txt file.",
                getDefaultValue: () => false
                );

        var rootCommand = new RootCommand("Command line application for solving Wordle problems.");
        rootCommand.Add(verboseOption);
        rootCommand.Add(wordFileOption);
        rootCommand.Add(wordLengthOption);
        rootCommand.Add(excludeOption);
        rootCommand.Add(includeOption);
        rootCommand.Add(knownOption);
        rootCommand.Add(saveToTxtOption);

        rootCommand.SetHandler(
                (verboseOptionValue, wordFileOptionValue, wordLengthOptionValue, excludeOptionValue,
                 includeOptionValue, knownOptionValue, saveToTxtOptionValue) =>
                {
                if (wordLengthOptionValue < 2)
                {
                Console.Error.WriteLine("Fatal error: Word length must be at least 2.");
                Environment.Exit(-1);
                }

                // Parse argument for valid letters and get pattern of letters to exclude
                List<char> excludedLetters = new List<char>();
                if (excludeOptionValue.Length > 0)
                {
                excludedLetters = excludeOptionValue
                .ToLower()
                .Split(",")
                .Where(arg => arg.Length == 1 && Char.IsAsciiLetterLower(arg[0]))
                .Select(s => s[0])
                .Distinct()
                .ToList();
                excludedLetters.Order();
                }
                if (excludedLetters.Count >= 26)
                {
                    Console.Error.WriteLine("Fatal error: All letters of the alphabet have been excluded.");
                    Environment.Exit(-1);
                }
                string letterGroup = "";
                if (excludedLetters.Any())
                    letterGroup = "[^" + string.Join("", excludedLetters) + "]";
                else
                    letterGroup = "[a-z]";

                // Parse argument for required letters
                List<char> requiredLetters = new List<char>();
                if (includeOptionValue.Length > 0)
                {
                    requiredLetters = includeOptionValue
                        .ToLower()
                        .Split(",")
                        .Where(arg => arg.Length == 1 && Char.IsAsciiLetterLower(arg[0]))
                        .Select(arg => arg[0])
                        .Distinct()
                        .ToList();
                    requiredLetters.Order();
                }

                // Parse argument for known positions
                char[] knownPositions = Enumerable.Repeat('-', wordLengthOptionValue).ToArray();
                int numKnownPositions = 0;
                if (knownOptionValue.Length > 0)
                {
                    Regex positionRegex = new Regex(@"^(\d+)([a-z])$");
                    IEnumerable<string> knownArgs = knownOptionValue
                        .ToLower()
                        .Split(",");
                    foreach (string arg in knownArgs)
                    {
                        Match m = positionRegex.Match(arg);
                        if (!m.Success || m.Groups.Count < 3)
                            continue;
                        int position = int.Parse(m.Groups[1].ToString()) - 1;
                        if (position >= 0 && position < wordLengthOptionValue)
                        {
                            numKnownPositions++;
                            char letter = m.Groups[2].ToString()[0];
                            knownPositions[position] = letter;
                        }
                    }
                }

                // Build regex pattern
                string regexString = "";
                if (numKnownPositions > 0)
                {
                    for (int i = 0; i < wordLengthOptionValue; i++)
                    {
                        if (knownPositions[i] != '-')
                            regexString += knownPositions[i];
                        else
                            regexString += letterGroup;
                    }
                }
                else
                {
                    regexString = $"{letterGroup}{{{wordLengthOptionValue}}}";
                }
                regexString = "^" + regexString + "$";

                if (verboseOptionValue)
                {
                    Console.WriteLine("Letters to exclude:");
                    if (excludedLetters.Any())
                        Console.WriteLine(string.Join(", ", excludedLetters));
                    else
                        Console.WriteLine("(none)");

                    Console.WriteLine("\nLetter group to be used in regular expression:");
                    Console.WriteLine(letterGroup);

                    Console.WriteLine("\nResulting regular expression:");
                    Console.WriteLine(regexString);

                    Console.WriteLine("\nRequired letters:");
                    if (requiredLetters.Any())
                        Console.WriteLine(string.Join(", ", requiredLetters));
                    else
                        Console.WriteLine("(none)");

                    Console.WriteLine();
                }

                SolveWordle(
                        wordFileOptionValue,
                        wordLengthOptionValue,
                        regexString,
                        requiredLetters,
                        saveToTxtOptionValue
                        );
                },
            verboseOption, wordFileOption, wordLengthOption, excludeOption,
            includeOption, knownOption, saveToTxtOption);

        rootCommand.Invoke(args);
    }
}
