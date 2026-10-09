bin_name := wordle_solver
bin := $(bin_name)

.PHONY: all
all: $(bin)

$(bin): $(bin_name).odin
	odin build $< -file -out:$(bin)

.PHONY: clean
clean:
	rm $(bin)
