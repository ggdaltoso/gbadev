# Builds every snippet. Each folder with a Makefile is one ROM.
SNIPPETS := $(patsubst %/Makefile,%,$(wildcard */Makefile))

.PHONY: all clean $(SNIPPETS)

all: $(SNIPPETS)

$(SNIPPETS):
	$(MAKE) -C $@

clean:
	for s in $(SNIPPETS); do $(MAKE) -C $$s clean; done
