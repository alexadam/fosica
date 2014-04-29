build: 
	gcc -g -pedantic -Wall -Wextra -lefence -std=c99 -o noiz utils/utils.c effects/effects.c fosica.c  -lpthread -lm -L/usr/local/lib -ljansson -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib
