# Makefile for L

CC:=cc
OBJDIR:=build
CFLAGS:=-c -I$(OBJDIR) -Isrc -Wall
LD:=$(CC) # Use $(CC) as linker so that I don't have to explicitly link to required
		  # libraries
LDFLAGS:=
LEX:=flex
YACC:=bison
VPATH:=src

OBJECTS:=l-mempool.o l-structures.o main.o lexer.o lexer.c parser.o parser.c l-parser-tokens.h

FULL_OBJS:=$(addprefix $(OBJDIR)/, $(OBJECTS))

all: $(OBJDIR)/l

$(OBJDIR)/l: $(FULL_OBJS)
	$(LD) $(LDFLAGS) $(filter-out %.h, $(filter-out %.c, $(FULL_OBJS))) -o $(OBJDIR)/l

$(OBJDIR)/lexer.c: l.lex
	$(LEX) -o $@ $<

$(OBJDIR)/parser.c: l.y
	$(YACC) -o $@ $<

$(OBJDIR)/l-parser-tokens.h: l.y
	$(YACC) $< -o /dev/null --defines=$@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -o$@

$(filter-out $(OBJDIR)/l-parser-tokens.h, $(FULL_OBJS)): $(OBJDIR)/l-parser-tokens.h $(wildcard $(OBJDIR)/*.h) | $(OBJDIR)
$(OBJDIR)/l-parser-tokens.h: | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -f $(OBJDIR)/*
	rmdir $(OBJDIR)
