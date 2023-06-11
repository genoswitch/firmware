// SPDX-License-Identifier: MIT

/* null.c
   This file exists to act as the 'source' for the combined binary (flashloader+main)
   It is passed as the paramater to add_executable
   > add_executable(${COMBINED} linker/null.c)

   We could register this target using add_custom_target,
   > add_custom_target(${COMBINED} ALL)
   but this would register it as a UTILITY target instead of an EXECUTABLE target.
   This means that the built file will not show as a launch target.

   Reference: https://stackoverflow.com/a/34234515
*/

int main() { return 0; }