nob: nob.c
	$(CC) -o nob nob.c
	@echo ""
	@echo "Bootstrapped nob"
	@echo "From now on, use ./nob to compile, it will rebuild automatically"
	@echo ""

.PHONY: run
run: nob
	./nob

.PHONY: clean
clean:
	rm -f nob
	rm -f nob.old
