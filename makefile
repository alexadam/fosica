build: 
	gcc  -std=c99 -o fosica fosica.c  -lpthread -lm -L/usr/local/lib -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib
