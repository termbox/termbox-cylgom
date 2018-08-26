NAME=keyboard
CC=gcc
FLAGS=-std=c99 -pedantic -Wall -Werror -g -static 
INCL=-I../
BIND=../../bin

.PHONY:all
all:$(NAME)

$(NAME).o:$(NAME).c
	@echo "building source object $@"
	@$(CC) $(INCL) $(FLAGS) -c -o $@ $<

$(NAME):$(NAME).o
	@echo "compiling $@"
	@$(CC) $(INCL) $(FLAGS) -o $(NAME) $(NAME).o $(BIND)/termbox.a

clean:
	@echo "cleaning workspace"
	@rm -rf ./*.o ./$(NAME)
