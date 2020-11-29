//
//  memmgr.c
//  memmgr
//
//  Created by William McCarthy on 17/11/20.
//  Copyright © 2020 William McCarthy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256


//-------------------------------------------------------------------
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  FILE* fbin = fopen("BACKING_STORE.bin", "rb");
  if (fbin == NULL) { fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n");  exit(FILE_ERROR);  }

  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt
  int address_count = 0;  
  int page_fault = 0;
  int TLB_hit = 0;
  //printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");

  // not quite correct -- should search page table before creating a new entry
      //   e.g., address # 25 from addresses.txt will fail the assertion
      // TODO:  add page table code
  unsigned pageList[256];
  for(int i =0; i<256; i++){
    pageList[i] = -99999999; // Not null because of NULL == 0
  }
      // TODO:  add TLB code
  unsigned TLB[16]; // Stores physical to check for duplicate values
  unsigned TLBframe[16];

  for(int i =0; i<16; i++){
    TLB[i] = -99999999; // Not Null because of NULL == 0
    TLBframe[i] = -99999999;
  }
  unsigned placeholder;
  unsigned phys_memory[256];
  bool hitTLB = false;
  
  while (feof(fadd) == 0) { // returns non-zero if EOF is found
    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add, buf, buf, &phys_add, buf, &value);  // read from file correct.txt
    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(  logic_add);
    offset = getoffset(logic_add);
    address_count++;
    // Check page number with TLB
    
    hitTLB = false;
    placeholder = -1;
    for(int i = 0 ; i<16 ; i++){
      if(TLB[i] == page){
        placeholder = i;
        hitTLB = true;
      }
    }

    // If hit, frame number is obtained from TLB
    if(hitTLB == true){
      TLB_hit++;
      physical_add = TLBframe[placeholder] * FRAME_SIZE + offset;
      //getpage_offset(TLB[placeholder]);
      //printf("PhysicalAdd = %d | phys_add = %d\n",physical_add, phys_add);
      assert(physical_add == phys_add);
    }
    // If TLB miss, page table consulted
    else{
    // If page table consulted, frame number is obtained from the page table.
      if(pageList[page] != -99999999){ //Not NULL because 0 == NULL 
      // Page table hit
        physical_add = pageList[page] * FRAME_SIZE + offset;
        TLB[page%16] = page;
        TLBframe[page%16] = pageList[page];
        assert(physical_add ==phys_add);
      }
      else{
      //printf("-----------Page Fault %d------------\n",frame+1);
        page_fault++;
        fseek(fbin, logic_add,SEEK_SET);
        //printf("\nFRAME#: %d| PAGE #: %d | OFFSET: %d\n",frame, page,offset);
        char* checkedValue;
        fread(&checkedValue,sizeof(char), 1,fbin);
        //getpage_offset(checkedValue);
        phys_memory[getpage(checkedValue)] = (int)checkedValue;

        //printf("PHYSMEM: %d\n", phys_memory[getpage((int)checkedValue)]);

        pageList[page] = frame;
        TLB[page%16] = page;
        TLBframe[page%16] = frame;
        physical_add = frame++ * FRAME_SIZE + offset;
        //printf("PhysicalAdd = %d | phys_add = %d\n",physical_add, phys_add);
        assert(physical_add == phys_add);
      }
    }
    printf("frame: %d logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed\n", frame, logic_add, page, offset, physical_add);
    if (frame % 5 == 0) { printf("\n"); }
    
  }
  fclose(fcorr);
  fclose(fadd);
  fclose(fbin);
  
 // printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");
  
 // printf("ALL logical ---> physical assertions PASSED!\n");
 // printf("!!! This doesn't work pass entry 24 in correct.txt, because of a duplicate page table entry\n");
 // printf("--- you have to implement the PTE and TLB part of this code\n");

//  printf("NOT CORRECT -- ONLY READ FIRST 20 ENTRIES... TODO: MAKE IT READ ALL ENTRIES\n");
address_count--; //extra due to EOF
  printf("Number of addresses: %d\n", address_count);
  printf("Page faults: %d\n", page_fault);
  printf("TLB Hits: %d\n", TLB_hit);
  double pagerate = (double)page_fault/(address_count);
  double tlbrate = (double)TLB_hit/(address_count);
  printf("Page Fault Rate: %f\n", pagerate);
  printf("TLB Hit Rate: %f\n", tlbrate);
  printf("\n\t\t...done.\n");
  return 0;
}
