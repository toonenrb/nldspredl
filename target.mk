# Adapted from http://make.mad-scientist.net/papers/multi-architecture-builds

# Remove all builtin rules
.SUFFIXES:

OBJDIR := _ARCH

# Rerun make in the object directory, using the makefile from the project root directory.
# Also set PJROOTDIR variable to use in that makefile.
MAKETARGET = $(MAKE) --no-print-directory -C $@ -f $(CURDIR)/Makefile \
                 PJROOTDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

% :: $(OBJDIR) ; :

.PHONY: clean
clean:
	rm -rf $(OBJDIR)
