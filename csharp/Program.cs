using System.CommandLine;
using System.CommandLine.Parsing;

static partial class Program
{
    public const string DefaultWordleWordFileName = "../wordlewords.txt";
    public const int DefaultWordleWordLength = 5;

    static int Main(string[] args)
    {
        Option<string> wordFileOption = new("--dict")
        {
            Description = "Path to a text file containing possible solutions",
            DefaultValueFactory = parseResult => DefaultWordleWordFileName,
        };
        Option<int> wordLengthOption = new("--length")
        {
            Description = "The length of the word to be found",
            DefaultValueFactory = parseResult => DefaultWordleWordLength,
        };
        Option<string> excludedOption = new("--excluded")
        {
            Description = "List of letters that are known to not be in the word. " +
                          "For example: --excluded mse",
            DefaultValueFactory = parseResult => "",
        };
        Option<string> requiredOption = new("--required")
        {
            Description = "List of letters that are known to be in the word but whose positions are unknown. " +
                          "For example: --required mse",
            DefaultValueFactory = parseResult => "",
        };
        Option<string> knownOption = new("--known")
        {
            Description = "A string showing the known letters. Mark unknown letters with an underscore. " +
                          "For example: --known mou_e",
            DefaultValueFactory = parseResult => "",
        };
        Option<bool> saveSolutionsOption = new("--save")
        {
            Description = "Save the potential solutions in a .txt file.",
            DefaultValueFactory = parseResult => false,
        };

        RootCommand rootCommand = new("Command line application for solving Wordle problems.")
        {
            wordFileOption,
            wordLengthOption,
            excludedOption,
            requiredOption,
            knownOption,
            saveSolutionsOption,
        };

        ParseResult parseResult = rootCommand.Parse(args);
        if (parseResult.Errors.Count > 0)
        {
            Console.WriteLine($"ERROR: Found {parseResult.Errors.Count} parse error(s).");
            foreach (ParseError parseError in parseResult.Errors)
            {
                Console.WriteLine($"{parseError.Message}");
            }
            return 1;
        }

        int wordLength = parseResult.GetValue(wordLengthOption);
        if (wordLength < 2)
        {
            Console.Error.WriteLine("ERROR: Word length must be at least 2.");
            return 1;
        }

        // Parse argument for letters to exclude
        HashSet<char> excludedLetterSet = [];
        string excludedLettersString = parseResult.GetValue(excludedOption) ?? "";
        if (excludedLettersString.Length > 0)
        {
            if (!excludedLettersString.All(char.IsLetter))
            {
                Console.Error.WriteLine("ERROR: Found invalid characters in the excluded letters string.");
                return 1;
            }

            excludedLetterSet = [.. excludedLettersString.ToLower()];
            if (excludedLetterSet.Count >= 26)
            {
                Console.Error.WriteLine("ERROR: All letters of the alphabet have been excluded.");
                return 1;
            }
        }

        // Parse argument for required letters
        HashSet<char> requiredLetterSet = [];
        string requiredLettersString = parseResult.GetValue(requiredOption) ?? "";
        if (requiredLettersString.Length > 0)
        {
            if (!requiredLettersString.All(char.IsLetter))
            {
                Console.Error.WriteLine("ERROR: Found invalid characters in the excluded letters string.");
                return 1;
            }

            requiredLetterSet = [.. requiredLettersString.ToLower()];
            if (requiredLetterSet.Count >= wordLength)
            {
                Console.Error.WriteLine("ERROR: More letters are required than are in the word.");
                return 1;
            }
        }

        // Parse argument for known positions
        string knownLetters = parseResult.GetValue(knownOption) ?? "";
        if (knownLetters.Length > 0)
        {
            if (knownLetters.Length != wordLength)
            {
                Console.Error.WriteLine("ERROR: The known letters string must be the same length as each word.");
                return 1;
            }

            if (!knownLetters.All(c => char.IsLetter(c) || c == '_'))
            {
                Console.Error.WriteLine("ERROR: The known letters string must contain letters or underscores.");
                return 1;
            }

            knownLetters = knownLetters.ToLower();
        }

        // Read in all the words from the file and filter accordingly
        string wordFilePath = parseResult.GetValue(wordFileOption) ?? DefaultWordleWordFileName;
        if (!File.Exists(wordFilePath))
        {
            Console.Error.WriteLine($"ERROR: {wordFilePath} does not exist.");
            return 1;
        }
        List<string> wordList = [.. File.ReadAllLines(wordFilePath)
            .Select(line => line.Trim().ToLower())
            .Where(line => line.Length == wordLength)];

        List<string> solutionList = [];

        foreach (string word in wordList)
        {
            int numRequiredLettersFound = 0;
            bool isValidWord = true;

            for (int i = 0; i < word.Length; ++i)
            {
                char letter = word[i];

                if (knownLetters.Length == wordLength)
                {
                    // Check if this position is a known letter.
                    // If so, also check if this letter matches the known letter.
                    char knownLetter = knownLetters[i];
                    if ((knownLetter != '_') && (letter != knownLetter))
                    {
                        isValidWord = false;
                        break;
                    }
                }

                if (excludedLetterSet.Contains(letter))
                {
                    isValidWord = false;
                    break;
                }

                if (requiredLetterSet.Contains(letter))
                {
                    ++numRequiredLettersFound;
                }
            }

            if (!isValidWord)
            {
                continue;
            }

            if (numRequiredLettersFound == requiredLetterSet.Count)
            {
                solutionList.Add(word);
            }
        }

        if (solutionList.Count > 0)
        {
            Console.WriteLine($"{solutionList.Count} possible solution(s):");
            foreach (string word in solutionList)
            {
                Console.WriteLine(word);
            }

            bool shouldSaveSolutions = parseResult.GetValue(saveSolutionsOption);
            if (shouldSaveSolutions)
            {
                File.WriteAllLines("solutions.txt", solutionList);
            }
        }

        return 0;
    }
}
