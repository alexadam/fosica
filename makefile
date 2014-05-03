build: 
	gcc -g -pedantic -Wall -Wextra -lefence -std=c99 -o noiz utils/utils.c utils/noiz_utils.c effects/effects.c cache/cache.c scripting/p_func.c fosica.c  -lpthread -lm -L/usr/local/lib -ljansson -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib
nw:
	gcc -w -lefence -std=c99 -o noiz utils/utils.c utils/noiz_utils.c effects/effects.c cache/cache.c scripting/p_func.c fosica.c  -lpthread -lm -L/usr/local/lib -ljansson -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib