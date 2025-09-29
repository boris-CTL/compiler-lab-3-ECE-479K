#include "debug_assert.h"
#include <stdio.h>

// Override glibc's assertion handler with ours
void __assert_fail(char const *assertion, char const *file, unsigned int line,
                   char const *function) noexcept(true) {
  fprintf(stderr, "%s:%s:%u: assertion %s failed\n", file, function, line,
          assertion);
  fflush(stderr);
  __builtin_debugtrap();
  __builtin_trap();
}
