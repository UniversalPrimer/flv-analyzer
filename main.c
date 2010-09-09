#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint8_t, uint16_t, etc.
#include <string.h> // strlen
#include <netinet/in.h> // hton, ntoh
#include "analyzer.h"

void usage();
void die();


int debug = 1;
char* program_name;
FILE* in_file;
struct flv_header flv_header;

void usage() {
  printf("Usage: %s [input.flv]\n", program_name);
  exit(-1);
}

int main(int argc, char** argv) {  

  program_name = argv[0];

  if(argc == 1) {
    in_file = stdin;
  } else {
    in_file = fopen(argv[1], "r");
    if(!in_file) {
      usage();
    }
  }


  init_analyzer(in_file);

  analyze();

  printf("\nFinished analyzing\n");
  
 


  return 0;
}
