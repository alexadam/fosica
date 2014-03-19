#include "utils.h"

char * localconcat(char *s1, char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc((len1+len2+1) * sizeof(char));//+1 for the zero-terminator
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

	char * fileContents;
	long inputFileSize;
	FILE * inputFile = fopen(fileName, "r");

	fseek(inputFile, 0, SEEK_END);
	inputFileSize = ftell(inputFile);
	rewind(inputFile);

	fileContents = malloc((inputFileSize + 1) * (sizeof(char)));

	fread(fileContents, sizeof(char), inputFileSize, inputFile);
	fclose(inputFile);

	fileContents[inputFileSize] = 0;

	return fileContents;
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

  char * result;
  size_t slen = strlen (str);

  if (len < slen)
    slen = len;

  result = malloc ((slen + 1) * sizeof(char));
  if (!result)
    return 0;

  result[slen] = '\0';
  memcpy (result, str + begin, slen);
  return result;
}
