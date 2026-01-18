all:
	@$(MAKE) -C mina-as
	@$(MAKE) -C simulator

clean:
	@$(MAKE) -C mina-as clean
	@$(MAKE) -C simulator clean

check: all
	@$(MAKE) -C simulator tests

.PHONY: all clean check
