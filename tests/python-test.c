#include <Python.h>

/**
 * python2.7-config --cflags
 *
 gcc -std=c99 -o python-test python-test.c -I/usr/include/python2.7 -I/usr/include/x86_64-linux-gnu/python2.7  -fno-strict-aliasing -D_FORTIFY_SOURCE=2 -g -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security  -DNDEBUG -g -fwrapv -O2 -Wall -Wstrict-prototypes
 *
 *
 */

int
main(int argc, char *argv[])
{
  Py_SetProgramName(argv[0]);  /* optional but recommended */
  Py_Initialize();
  PyRun_SimpleString("from time import time,ctime\n"
                     "print 'Today is',ctime(time())\n");
  Py_Finalize();
  return 0;
}
