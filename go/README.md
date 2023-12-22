# Wordle Solver in Go

## Usage

```
$ go build wordlesolver.go
$ ./wordlesolver -exclude m,o,a,c -include u -known 1s,5e 
```

Help output:
```
$ ./wordlesolver -help
Usage of ./wordlesolver:
  -dict string
    	Path to text file containing a list of words. (default "wordlewords.txt")
  -exclude string
    	List of letters known to not be in the word. Separate multiple with a comma: -exclude m,s,e
  -include string
    	List of letters known to be in the word but whose positions are unknown. Separate multiple with a comma: -include m,s,e
  -known string
    	List of known positions and letters. Separate multiple with a comma: -known 1m,2o,3u
  -length int
    	The length of the word to be found. (default 5)
  -save
    	Save the potential solutions in a .txt file.
  -verbose
    	Show how the user's arguments were interpreted.
```
