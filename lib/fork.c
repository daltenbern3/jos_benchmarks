// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
  void *addr = (void *) utf->utf_fault_va;
  uint32_t err = utf->utf_err;
  int r;

  // Check that the faulting access was (1) a write, and (2) to a
  // copy-on-write page.  If not, panic.
  // Hint:
  //   Use the read-only page table mappings at vpt
  //   (see <inc/memlayout.h>).

  // LAB 4: Your code here.
	
  // check if access is write and to a copy-on-write page.
  if (!(err & FEC_WR) || !(uvpt[PGNUM(addr)] & PTE_COW))
    panic("faulting access was not a write.1st:%08x 2nd:%08x\n",!(err & FEC_WR),!(uvpt[PGNUM(addr)] & PTE_COW));

  // Allocate a new page, map it at a temporary location (PFTEMP),
  // copy the data from the old page to the new page, then move the new
  // page to the old page's address.
  // Hint:
  //   You should make three system calls.
  //   No need to explicitly delete the old page's mapping.

  // LAB 4: Your code here.
  if (sys_page_alloc(0, PFTEMP, PTE_W | PTE_U | PTE_P))
    panic("pgfault: no phys mem");

  // copy data to the new page from the source page.
  void *rounding = (void *)ROUNDDOWN(addr, PGSIZE);
  memmove(PFTEMP, rounding, PGSIZE);

  // change mapping for the faulting page.
  int rv;
  if ((rv = sys_page_map(0,PFTEMP,0,rounding,PTE_W | PTE_U | PTE_P)))
    panic("pgfault: map error:%e",rv);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
  int r;

  // LAB 4: Your code here.

  pte_t pte = uvpt[pn];
  void *va = (void *)(pn << PGSHIFT);

  // If the page is writable or copy-on-write,
  // the mapping must be copy-on-write ,
  // otherwise the new environment could change this page.
  int rv;
  cprintf("DUPPAGE: envid:%08x\n",envid);
  if((pte & PTE_W) || (pte & PTE_COW)) {
    if((rv=sys_page_map(0,va,envid,va,PTE_COW | PTE_U | PTE_P)))
      panic("duppage: map cow error:%e",rv);
        
    // Change permission of the page in this environment to copy-on-write.
    // Otherwise the new environment would see the change in this environment.
    if(sys_page_map(0,va,0,va,PTE_COW | PTE_U | PTE_P))
      panic("duppage: change perm error");
  }else if(sys_page_map(0,va, envid, va,PTE_U | PTE_P))
    panic("duppage: map ro error");

  return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
  // LAB 4: Your code here.
    
  set_pgfault_handler(pgfault);

  envid_t envid = sys_exofork();
  cprintf("fork: envid:%08x\n",envid);
  if (envid < 0) {
    panic("failed to create child");
  }else if (envid == 0) {
    thisenv = &envs[ENVX(sys_getenvid())];
    return 0;
  }

  int ipd;
  for (ipd = 0; ipd != PDX(UTOP); ++ipd) {
    if (!(uvpd[ipd] & PTE_P))
      continue;

    int ipt;
    for (ipt = 0; ipt != NPTENTRIES; ++ipt) {
      unsigned pn = (ipd << 10) | ipt;
      if (pn == PGNUM(UXSTACKTOP - PGSIZE)) {
        if (sys_page_alloc(envid,(void *)(UXSTACKTOP - PGSIZE),PTE_W |      		    PTE_U | PTE_P))
          panic("fork: no phys mem for xstk");
        continue;
      }
      if (uvpt[pn] & PTE_P)
        duppage(envid, pn);
    }
  }

  if (sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall))
    panic("fork: cannot set pgfault upcall");

  if (sys_env_set_status(envid, ENV_RUNNABLE))
    panic("fork: cannot set env status");

  return envid;
}

// Challenge!
int
sfork(void)
{
  panic("sfork not implemented");
  return -E_INVAL;
}
