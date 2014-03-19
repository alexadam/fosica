
/**
 *
 gcc -std=c99 -o json-test ../utils.c json-test.c -L/usr/local/lib -ljansson -Wl,-rpath -Wl,/usr/local/lib
 *
 *
 */

#include <string.h>
#include <jansson.h>
#include "../utils.h"

int main(int argc, char *argv[])
{
	char * text = readFileToBuffer("test.json");

	json_t *root;
	json_error_t error;

	root = json_loads(text, 0, &error);
	free(text);

	if(!root) {
	    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
	    return 1;
	}

	json_t * tracks = json_object_get(root, "tracks");

	if(!json_is_array(tracks)) {
	    fprintf(stderr, "error: root is not an array\n");
	    json_decref(root);
	    return 1;
	}

	for(int i = 0; i < json_array_size(tracks); i++) {
		json_t * track = json_array_get(tracks, i);
		if (!json_is_object(track)) {
			fprintf(stderr, "error: track %d is not an object\n", i + 1);
			json_decref(root);
			return 1;
		}

		json_t * trackName = json_object_get(track, "trackName");
		if (!json_is_string(trackName)) {
			fprintf(stderr, "error: trackName %d: is not a string\n", i + 1);
			json_decref(root);
			return 1;
		}

		char * trackNameText = json_string_value(trackName);
		printf("TRACK NAME %s\n", trackNameText);

		json_t * sequences = json_object_get(track, "sequences");
		if (!json_is_array(sequences)) {
			fprintf(stderr, "error: commit %d: sequences is not an array\n", i + 1);
			json_decref(root);
			return 1;
		}

		for (int j = 0; j < json_array_size(sequences); j++) {
			json_t * sequence = json_array_get(sequences, j);

			if (!json_is_object(sequence)) {
				fprintf(stderr, "error: sequence data %d is not an object\n", j + 1);
				json_decref(root);
				return 1;
			}

			json_t * seqStart = json_object_get(sequence, "start");
			if (!json_is_integer(seqStart)) {
				fprintf(stderr, "error: commit %d: seqStart is not a int\n", j + 1);
				json_decref(root);
				return 1;
			}

			int seqStartInt = json_integer_value(seqStart);
			printf("SEQ Start %d\n", seqStartInt);

			json_t * seqStop = json_object_get(sequence, "stop");
			if (!json_is_integer(seqStop)) {
				fprintf(stderr, "error: commit %d: seqStop is not a int\n", j + 1);
				json_decref(root);
				return 1;
			}

			int seqStopInt = json_integer_value(seqStop);
			printf("SEQ Start %d\n", seqStopInt);

			json_t * seqInstr = json_object_get(sequence, "instructions");
			if (!json_is_string(seqInstr)) {
				fprintf(stderr, "error: commit %d: seqInstr is not a string\n", j + 1);
				json_decref(root);
				return 1;
			}

			char * seqInstrText = json_string_value(seqInstr);
			printf("seq instr %s\n", seqInstrText);

		}

	}

	return 0;
}
