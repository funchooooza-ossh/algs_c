.PHONY: run

num ?= 0
args ?=

run:
ifeq ($(num),0)
	@echo "supported lab nums 1-5"
else
	$(MAKE) run-$(num)
endif


.PHONY: run-1
run-1:
	@gcc -Wall -Wextra -g3 lab1/main.c -o output/main1
	@output/main1


.PHONY: run-2
run-2:
	@gcc -Wall -Wextra -g3 lab2/main.c -o output/main2
	@output/main2

.PHONY: run-3
run-3:
	@gcc -Wall -Wextra -g3 lab3/main.c -o output/main3
	@output/main3 $(if $(args),$(args),./lab3/texts/1ws4610.txt)


.PHONY: run-4
run-4:
	@gcc -Wall -Wextra -g3 lab4/main.c -o output/main4
	@output/main4 $(if $(args),$(args), lab4/glyph/)
