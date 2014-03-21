build: 
	gcc -g -Wall -lefence -std=c99 -o fosica utils.c effects.c fosica.c  -lpthread -lm -L/usr/local/lib -ljansson -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib
