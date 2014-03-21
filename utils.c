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

char * str_replace( const char *string, const char *substr, const char *replacement ){
  char *tok = NULL;
  char *newstr = NULL;
  char *oldstr = NULL;
  /* if either substr or replacement is NULL, duplicate string a let caller handle it */
  if ( substr == NULL || replacement == NULL ) return strdup (string);
  newstr = strdup (string);

  while ( (tok = strstr ( newstr, substr ))){
    oldstr = newstr;
    newstr = malloc ( strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) + 1 );

    /*failed to alloc mem, free old string and return NULL */
    if ( newstr == NULL ){
      free (oldstr);
      return NULL;
    }

    memcpy ( newstr, oldstr, tok - oldstr );
    memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
    memcpy ( newstr + (tok - oldstr) + strlen( replacement ), tok + strlen ( substr ), strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
    memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) , 0, 1 );

    free (oldstr);
  }

  return newstr;
}

char ** str_split(char* a_str, const char a_delim, int * elemCount)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            (*elemCount)++;
            last_comma = tmp;
        }
        tmp++;
    }

    (*elemCount)++;

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char * at = strdup(a_str);
        char* token = strtok(at, delim);
        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            printf("AICI 3\n");
            token = strtok(0, delim);
        }
        assert(idx == count - 1); //TODO cannot handle empty list
        *(result + idx) = 0;

        free(at);
//        free(token);
    }

    return result;
}

int string2int(char * input) {
	return (int)string2long(input);
}

char * int2string(int input) {
	char * intBuf = malloc(32 * sizeof(char));
	snprintf (intBuf, 32, "%d", input);
	return intBuf;
}

char * ulint2string(unsigned long int input) {
	char * intBuf = malloc(64 * sizeof(char));
	snprintf (intBuf, 64, "%ul", input);
	return intBuf;
}

long string2long(char * input) {
	return strtol(input, NULL, 0);
}

float string2float(char * input) {
	return strtof(input, NULL);
}
