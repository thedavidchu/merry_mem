CPPC=g++
CPPFLAGS=-Wall -std=c++20 -g

all: clean sequential

sequential:
	@rm -rf $@.out
	@$(CPPC) $(CPPFLAGS) src/sequential/{sequential,main}.cpp -o $@.out

clean:
	@rm -rf **/*.{o,out}
