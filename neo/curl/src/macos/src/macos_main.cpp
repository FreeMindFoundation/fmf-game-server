/* =========================================================================
Copyright (C) 2001 Eric Lavigne	Permission is granted to anyone to use this software for any purpose on any	computer system, and to redistribute it freely, subject to the following restrictions:
- The author is not responsible for the consequences of use of this software, no matter how awful, even if they arise from defects in it.
- The origin of this software must not be misrepresented, either by	explicit claim or by omission.
- You are allowed to distributed modified copies of the software, in source	and binary form, provided they are marked plainly as altered versions, and	are not misrepresented as being the original software.
========================================================================= */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Memory.h>
#include <GUSICommandLine.h>
#include <stdlib.h>

/* ========================================================================= */
DECLARE_MAIN(curl)
REGISTER_MAIN_STARTREGISTER_MAIN(curl)
REGISTER_MAIN_END
/* ========================================================================= */

int main(){	
	::MaxApplZone();
	for (int i = 1; i <= 10; i++)
		::MoreMasters();
	(void) exec_commands();
	return 0;
}
