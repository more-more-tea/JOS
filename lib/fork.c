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
    if (err & FEC_WR || !(vpd[PDX(addr)] & PTE_P) || !(vpt[PGNUM(addr)] & PTE_COW))
        panic("pgfault: not a write to a copy-on-write page.");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	// panic("pgfault not implemented");
    if ((r = sys_page_alloc(0, (void *)PFTEMP, PTE_U | PTE_P | PTE_W)) < 0)
        panic("pgfault: %e.", r);

    addr = ROUNDDOWN(addr, PGSIZE);
    memmove(PFTEMP, addr, PGSIZE);
    if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_U | PTE_P | PTE_W)) < 0)
        panic("sys_page_map: %e.", r);
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
	// panic("duppage not implemented");
    void *va = (void *)(pn * PGSIZE);

    // if pp is NULL, it means the virtual address is not mapped yet,
    // that is allowed.
    pte_t pte = vpt[PGNUM(va)];

    if ((pte & PTE_W) || (pte & PTE_COW)) {
        if ((r = sys_page_map(0, va, envid, va, PTE_U | PTE_P | PTE_COW)) < 0)
            panic("duppage: child COW page remapping fail %e.", r);

        if ((r = sys_page_map(0, va, 0, va, PTE_U | PTE_P | PTE_COW)) < 0)
            panic("duppage: parent COW page remapping fail %e.", r);
    } else {
        if ((r = sys_page_map(0, va, envid, va, PTE_U | PTE_P)) < 0)
            panic("duppage: child page remapping fail %e.", r);
    }

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
	// panic("fork not implemented");
    set_pgfault_handler(pgfault);

    int rc = 0;

    // create a child env
    envid_t child = sys_exofork();
    if (child < 0)
        panic("Fail to create a child environment: %e", child);

    if (child == 0) {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    // map memory address copy-on-write
    // we need only map to address below UTOP
    // memory above UTOP is mapped in function env_setup_vm in
    // kern/env.c
    // memory below UTEXT is empty
    uint32_t addr;
    for (addr = UTEXT; addr < USTACKTOP; addr += PGSIZE)
        if ((vpd[PDX(addr)] & PTE_P) && (vpt[PGNUM(addr)] & PTE_P) && (vpt[PGNUM(addr)] & PTE_U))
            duppage(child, PGNUM(addr));

    extern void _pgfault_upcall(void);
    sys_env_set_pgfault_upcall(child, _pgfault_upcall);
    if (_pgfault_upcall != NULL)
        // remap a new user exception stack
        if ((rc = sys_page_alloc(child, (void *)(UXSTACKTOP - PGSIZE), PTE_SYSCALL)) < 0)
            panic("sys_page_alloc: %e.", rc);

    if ((rc = sys_env_set_status(child, ENV_RUNNABLE)) < 0)
        panic("sys_env_set_status: %e", rc);

    return child;
}

// Challenge!
int
sfork(void)
{
	// panic("sfork not implemented");
	return -E_INVAL;
}
