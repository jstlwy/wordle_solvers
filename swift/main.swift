import Foundation

// Parse command line arguments
let verboseArg      = "--verbose"
let wordFilePathArg = "-list"
let requireArg      = "-require"
let excludeArg      = "-exclude"
let knownArg        = "-known"

let letterList = #/[a-z](?:,[a-z])*/#
let indexAndLetterList = #/[1-5][a-z](?:,[1-5][a-z])*/#

var argDict: [String: Int] = [
    verboseArg: -1,
    wordFilePathArg: -1,
    requireArg: -1,
    excludeArg: -1,
    knownArg: -1,
]

let args = CommandLine.arguments
for (i, arg) in args.enumerated() {
    if argDict.keys.contains(arg) {
        argDict[arg] = i
    }
}

// Get all flags
func flagWasFound(arg: String, argDict: inout [String: Int]) -> Bool {
    guard let idx = argDict[arg] else {
        return false
    }
    argDict.removeValue(forKey: arg)
    return idx >= 0
}

let verbose = flagWasFound(arg: verboseArg, argDict: &argDict)

// Get parameters for options
func getArgParam(arg: String, argDict: inout [String: Int], cliArgs: [String]) -> String? {
    guard let argIdx = argDict[arg] else {
        return nil
    }
    let paramIdx = argIdx + 1
    if (paramIdx < 1) || (paramIdx >= cliArgs.count) {
        return nil
    }
    let param = cliArgs[paramIdx]
    if param.first == "-" {
        return nil
    }
    argDict.removeValue(forKey: arg)
    return param
}

// Get path to text file containing a list of words
// and confirm whether the file exists
let wordFilePath = getArgParam(arg: wordFilePathArg, argDict: &argDict, cliArgs: args) ?? "../wordlewords.txt"
let fileManager = FileManager.default
if !fileManager.fileExists(atPath: wordFilePath) {
    print("Error: Invalid word file path.")
    exit(1)
}

// Get required letters and letters to exclude
func getLetterSet(arg: String, argDict: inout [String: Int], cliArgs: [String]) -> Set<Character> {
    var letterSet: Set<Character> = []
    guard let param = getArgParam(arg: arg, argDict: &argDict, cliArgs: cliArgs) else {
        return letterSet
    }
    if let _ = try? letterList.wholeMatch(in: param) {
        let letters = param.split(separator: ",", omittingEmptySubsequences: true)
        for letter in letters {
            if let c = letter.first, c.isLetter {
                letterSet.insert(c)
            }
        }
    } else {
        print("Invalid parameter format: \(param)")
        exit(1)
    }
    return letterSet
}

let requireLetterSet = getLetterSet(arg: requireArg, argDict: &argDict, cliArgs: args)
let excludeLetterSet = getLetterSet(arg: excludeArg, argDict: &argDict, cliArgs: args)

if verbose {
    if (!requireLetterSet.isEmpty) {
        print("Letters to require:")
        print(requireLetterSet)
    }
    if (!excludeLetterSet.isEmpty) {
        print("Letters to exclude:")
        print(excludeLetterSet)
    }
}

if excludeLetterSet.count >= 26 {
    print("Error: All letters of the alphabet have been excluded.")
    exit(1)
}
if !requireLetterSet.isDisjoint(with: excludeLetterSet) {
    print("Error: The set of letters to require and the set of letters to exclude have one or more letters in common.")
    exit(1)
}

// Get known positions
var knownLetters: [Character] = Array(repeating: "*", count: 5)
if let knownParam = getArgParam(arg: knownArg, argDict: &argDict, cliArgs: args) {
    if let _ = try? indexAndLetterList.wholeMatch(in: knownParam) {
        let knownArgs = knownParam.split(separator: ",", omittingEmptySubsequences: true)
        if knownArgs.count > 4 {
            print("Error: Too many known positions.")
            exit(1)
        }
        for knownArg in knownArgs {
            if knownArg.count != 2 {
                continue
            }
            guard let index: Int = knownArg.first?.wholeNumberValue else {
                continue
            }
            if (index < 1) || (index > 5) {
                continue
            }
            let second = knownArg.index(knownArg.startIndex, offsetBy: 1)
            let letter: Character = knownArg[second]
            if !letter.isLetter {
                continue
            }
            knownLetters[index-1] = letter
        }
    } else {
        print("Error: Invalid parameter format: \(knownParam)")
        exit(1)
    }
}

if verbose {
    print("Known letters:")
    print(knownLetters)
}

let knownLetterSet = knownLetters.filter({ $0.isLetter })
if !requireLetterSet.isDisjoint(with: knownLetterSet) ||
   !excludeLetterSet.isDisjoint(with: knownLetterSet) {
    print("Error: One or more of the known letters was supplied to another argument.")
    exit(1)
}

// Get all words that match arguments
let handle = FileHandle(forReadingAtPath: wordFilePath)
guard let lines = handle?.bytes.lines else {
    print("Unable to open \(wordFilePath).")
    exit(1)
}
for try await line in lines {
    //let word = line.trimmingCharacters(in: .whitespaces)
    if line.count != 5 {
        continue
    }
    if !requireLetterSet.isSubset(of: line) {
        continue
    }
    if !excludeLetterSet.isDisjoint(with: line) {
        continue
    }
    if knownLetterSet.isEmpty {
        print(line)
        continue
    }
    var isValidWord: Bool = true
    for (knownLetter, currentLetter) in zip(knownLetters, line) {
        if knownLetter == "*" {
            continue
        }
        if knownLetter != currentLetter {
            isValidWord = false
            break
        }
    }
    if isValidWord {
        print(line)
    }
}

