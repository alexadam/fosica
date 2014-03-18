#include "utils.h"

char * localconcat(char *s1, char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);//+1 to copy the null-terminator
    return result;
}


// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char * trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

char * readFileToBuffer(char * fileName) {
	char *buffer = NULL;
	int string_size, read_size;
	FILE *handler = fopen(fileName, "r");

	if (handler) {
		//seek the last byte of the file
		fseek(handler, 0, SEEK_END);
		//offset from the first to the last byte, or in other words, filesize
		string_size = ftell(handler);
		//go back to the start of the file
		rewind(handler);

		//allocate a string that can hold it all
		buffer = (char*) malloc(sizeof(char) * (string_size + 1));
		//read it all in one operation
		read_size = fread(buffer, sizeof(char), string_size, handler);
		//fread doesnt set it so put a \0 in the last position
		//and buffer is now officialy a string
		buffer[string_size + 1] = '\0';

		if (string_size != read_size) {
			//something went wrong, throw away the memory and set
			//the buffer to NULL
			free(buffer);
			buffer = NULL;
		}
	}

	return buffer;
}

void copy_string(char * target, char * source)
{
   while(*source) {
      *target = *source;
      source++;
      target++;
   }
   *target = '\0';
}

char * substring(const char * str, size_t begin, size_t len) {
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len)) {
	  return 0;
  }

  char *result;
  size_t slen = strlen (str);

  if (len < slen)
    slen = len;

  result = (char *) malloc (slen + 1);
  if (!result)
    return 0;

  result[slen] = '\0';
  return (char *) memcpy (result, str, slen);

}
