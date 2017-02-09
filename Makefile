all:
	gcc -o ibecho ibecho.c -libverbs
.PHONY: clean

clean: 
	rm ibecho
