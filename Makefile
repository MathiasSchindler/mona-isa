
all:
	@$(MAKE) -C mina-as
	@$(MAKE) -C simulator

clean:
	@$(MAKE) -C mina-as clean
	@$(MAKE) -C simulator clean

test: all
	@$(MAKE) -C mina-as status
	@$(MAKE) -C simulator tests
	@python3 tools/spec_vs_sim.py --tests-passed

check: test

.PHONY: all clean test check
