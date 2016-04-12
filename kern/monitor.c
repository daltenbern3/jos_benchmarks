// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>
#include <kern/env.h>

#define CMDBUF_SIZE 80 // enough for one VGA text line


struct Command {
  const char *name;
  const char *desc;
  // return -1 to force monitor to exit
  int (*func)(int argc, char **argv, struct Trapframe * tf);
};

static struct Command commands[] = {
  { "help",      "Display this list of commands",        mon_help       },
  { "info-kern", "Display information about the kernel", mon_infokern   },
  { "backtrace", "See the backtrace"                   , mon_backtrace  },
  { "smappings", "Show mappings of a virtual address",   mon_smappings  },
  { "eperm",     "Edit Permissions: eperm va [perm]",    mon_eperm      },
  { "dumprng",   "dumprng -p -v add1 add2",              mon_dumprng    },
  { "continue",  "Continue execution from breakpoint",   mon_continue   },
  { "step",      "step to next instruction",             mon_step       },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
  int i;

  for (i = 0; i < NCOMMANDS; i++)
    cprintf("%s - %s\n", commands[i].name, commands[i].desc);
  return 0;
}

int
mon_infokern(int argc, char **argv, struct Trapframe *tf)
{
  extern char _start[], entry[], etext[], edata[], end[];

  cprintf("Special kernel symbols:\n");
  cprintf("  _start                  %08x (phys)\n", _start);
  cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
  cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
  cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
  cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
  cprintf("Kernel executable memory footprint: %dKB\n",
          ROUNDUP(end - entry, 1024) / 1024);
  return 0;
}


int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
  uintptr_t *ebp = (uintptr_t *)read_ebp();
  int i = -8;
  //struct Eipdebuginfo *info = (struct Eipdebuginfo *)ebp-256*sizeof(uint32_t);
  while(ebp[i+1] != 0){
    //debuginfo_eip((uintptr_t)ebp[i+1],info);
    cprintf("ebp %08x eip %08x args %08x %08x %08x %08x %08x\n",(uint32_t)ebp+i*sizeof(uint32_t),ebp[i+1],ebp[i+3],ebp[i+4],ebp[i+5],ebp[i+6],ebp[i+7]);
    i += 8;
  }
  return 0;
}

int
mon_smappings(int argc, char **argv, struct Trapframe *tf)
{
  size_t i;
  char* va;
  char perm[20];
  pte_t* pte;
  if((argv[1][0] != '0' || (argv[1][1] != 'x' && argv[1][1] != 'X')) || strlen(argv[1]) > 10){
    cprintf("Please enter a valid hex address\n");
    return 0;
  }
  cprintf("Virtual Address  Physical Address Base  Permissions\n");
  for(i=1; i < argc; i++){
    strcpy(perm,"");
    va = (char*)strtol(argv[i],&va,16);
    pte = pgdir_walk(kern_pgdir, va, 0);
    if(pte == NULL)
      cprintf("0x%08x:      No Directory Entry Made\n",va);
    else if(pte==0){
      cprintf("0x%08x:      No Page Table Entry\n",va);
    }else{
      if(*pte&PTE_P) strcat(perm,"PTE_P ");
      if(*pte&PTE_U) strcat(perm,"PTE_U ");
      if(*pte&PTE_W) strcat(perm,"PTE_W ");
      cprintf("0x%08x:      0x%08x             %s\n",va,PTE_ADDR(*pte),perm);
    }
  }
  return 0;
}

int
mon_eperm(int argc, char **argv, struct Trapframe *tf)
{
  size_t i;
  char* va;
  size_t perm = 0;
  pte_t* pte;
  if((argv[1][0] != '0' || (argv[1][1] != 'x' && argv[1][1] != 'X')) || strlen(argv[1]) > 10){
    cprintf("Please enter a valid hex address\n");
    return 0;
  }
  va = (char*)strtol(argv[1],&va,16);
  pte = pgdir_walk(kern_pgdir, va, 0);
  if(pte == NULL)
    cprintf("No Directory Entry Made\n",va);
  else if(pte==0){
    cprintf("No Page Table Entry\n",va);
  }else{
    for(i=2; i < argc; i++){
      if(argv[i][4] == 'P') perm = perm|PTE_P;
      else if(argv[i][4] == 'U') perm = perm|PTE_U;
      else if(argv[i][4] == 'W') perm = perm|PTE_W;
      else cprintf("Please use valid permissions. %s is not valid(PTE_P,PTE_U,PTE_W)",argv[i]);
    }
    page_insert(kern_pgdir, pa2page(PTE_ADDR(*pte)), va, perm);
    cprintf("Permissions have been changed!0x%08x\n",PTE_ADDR(*pte));
  }
  return 0;
}

int
mon_dumprng(int argc, char **argv, struct Trapframe *tf)
{
  if(argv[1][0] != '-'){
    cprintf("Please use a -p(Physical) or -v(Virtual) option");
    return 0;
  }
  if((argv[2][0] != '0' || (argv[2][1] != 'x' && argv[2][1] != 'X')) || strlen(argv[2]) > 10
       ||
     (argv[3][0] != '0' || (argv[3][1] != 'x' && argv[3][1] != 'X')) || strlen(argv[3]) > 10){
    cprintf("Please enter a valid hex address\n");
    return 0;
  }
  char* addr1;
  char* addr2;
  physaddr_t pa1,pa2;
  char* va;
  cprintf("PDX(UPAGES):%d\n",PDX(UPAGES));
  if(argv[1][1] == 'p'){
    pa1 = (physaddr_t)strtol(argv[2],&va,16);
    addr1 = KADDR(pa1);
    pa2 = (physaddr_t)strtol(argv[3],&va,16);
    addr2 = (char*)KADDR(pa2);
    for(; addr1 < addr2; addr1++){
      cprintf("0x%08x:  %08x   %c\n",pa1++,*addr1,*addr1);
    }
  }else if(argv[1][1] == 'v'){
    addr1 = (char*)strtol(argv[2],&va,16);
    addr2 = (char*)strtol(argv[3],&va,16);
    for(; addr1<addr2; addr1++){
      cprintf("0x%08x:  %08x   %c\n",addr1,*addr1,*addr1);
    }
  }else{
    cprintf("Please use a -p(Physical) or -v(Virtual) option");
  }
  return 0;
}

int
mon_continue(int argc, char **argv, struct Trapframe *tf)
{
  env_pop_tf(tf);
  return 0;
}

int
mon_step(int argc, char **argv, struct Trapframe *tf)
{
  tf->tf_eflags = tf->tf_eflags | FL_TF;
  env_pop_tf(tf);
  return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
  int argc;
  char *argv[MAXARGS];
  int i;

  // Parse the command buffer into whitespace-separated arguments
  argc = 0;
  argv[argc] = 0;
  while (1) {
    // gobble whitespace
    while (*buf && strchr(WHITESPACE, *buf))
      *buf++ = 0;
    if (*buf == 0)
      break;

    // save and scan past next arg
    if (argc == MAXARGS-1) {
      cprintf("Too many arguments (max %d)\n", MAXARGS);
      return 0;
    }
    argv[argc++] = buf;
    while (*buf && !strchr(WHITESPACE, *buf))
      buf++;
  }
  argv[argc] = 0;

  // Lookup and invoke the command
  if (argc == 0)
    return 0;
  for (i = 0; i < NCOMMANDS; i++)
    if (strcmp(argv[0], commands[i].name) == 0)
      return commands[i].func(argc, argv, tf);
  cprintf("Unknown command '%s'\n", argv[0]);
  return 0;
}

void
monitor(struct Trapframe *tf)
{
  char *buf;

  cprintf("Welcome to the JOS kernel monitor!\n");
  cprintf("Type 'help' for a list of commands.\n");

  if (tf != NULL)
    print_trapframe(tf);

  while (1) {
    buf = readline("K> ");
    if (buf != NULL)
      if (runcmd(buf, tf) < 0)
        break;
  }
}
