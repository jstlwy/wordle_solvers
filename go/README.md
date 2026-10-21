# Wordle Solver in Go

## Usage

```
$ go build wordlesolver.go
$ ./wordlesolver -exclude moac -require u -known s___e
```

Help output:
```
$ ./wordlesolver -help
Usage of ./wordlesolver:
  -dict string
    	A path to a text file containing a list of words. (default "../wordlewords.txt")
  -exclude string
    	A list of letters known to not be in the word. Example: -exclude mse
  -known string
    	A list of known positions. Mark unknown positions with an underscore. Example: -known m__se
  -require string
    	A list of letters known to be in the word but whose positions are unknown. Example: -require mse
  -save
    	Save the potential solutions in a .txt file.
```
