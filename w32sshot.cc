#define _WIN32_WINNT 0x501
#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <psapi.h>
#include <unistd.h>
#include <assert.h>
#include "W32Process.h"
	
/* ripped from vexllvm code */
#define VEXLLVM_ARCH_I386	3

#define INIT_FILE(x)				\
	sprintf(f_path, "%s/" #x, path);	\
	f = fopen(f_path, "w");			\
	if (!f) return false;

bool dumpProcess(W32Process* w32p, const char* path)
{
	FILE*	f;
	char	f_path[MAX_PATH];

	if (mkdir(path) != 0) {
		std::cerr << "FAILED TO MAKE PATH: " << path << '\n';
		return false;
	}

	INIT_FILE(binpath);
	fwrite(	w32p->getExe().c_str(),
		w32p->getExe().size(),
		1,
		f);
	fclose(f);

	/* XXX: argv */
	/* XXX: arch */
	uint32_t arch = VEXLLVM_ARCH_I386;
	INIT_FILE(arch);
	fwrite(&arch, sizeof(arch), 1, f);
	fclose(f);

	/* XXX: entry */

	/* XXX: regs */
	INIT_FILE(regs)
	HANDLE	h_thread;
	CONTEXT	ctx;
//	h_thread = OpenThread(
//	the way to do this is to use thread32first() + snapshot.
//	awful api
	assert (0 == 1);
	if (GetThreadContext(h_thread, &ctx) == false)
		return false;
	fclose(f);

	/* XXX: syms */

	/* XXX: mapinfo */
	/* XXX: maps/ */

	return true;
}

int main(int argc, char* argv[])
{
	DEBUG_EVENT	de;
	DWORD		pids[4096], used_bytes;
	W32Process	*w32p;
	BOOL		ok;

	ok = EnumProcesses(pids, sizeof(pids), &used_bytes);
	if (!ok) {
		std::cerr << "Failed to enum processes\n";
		return -1;
	}

	for (unsigned i = 0; i < used_bytes/sizeof(DWORD); i++) {
		w32p = W32Process::create(pids[i]);
		if (w32p == NULL)
			continue;

		if (w32p->getNumMods() <= 0)
			goto ignore;

		if (argc > 1) {
			if (strcmp(w32p->getExe().c_str(), argv[1]) == 0)
				break;
		} else
			std::cout << w32p->getExe() << '\n';
ignore:
		delete w32p;
		w32p = NULL;
	}

	if (argc < 2)
		return 0;

	if (w32p == NULL) {
		std::cerr << "Could not find " << argv[1] << '\n';
		return -2;
	}

	std::cout << "Debugging: " << w32p->getExe() << '\n';
	ok = DebugActiveProcess(w32p->getPID());
	if (!ok) {
		std::cerr << "Failed to debug process\n";
		return -2;
	}

	std::cerr << "Breaking process\n";
	ok = DebugBreakProcess(w32p->getHandle());
	WaitForDebugEvent(&de, INFINITE);

	dumpProcess(w32p, "snapshot");

	std::cerr << "Restoring control to process\n";
	ok = DebugActiveProcessStop(w32p->getPID());

	delete w32p;
	return 0;
}