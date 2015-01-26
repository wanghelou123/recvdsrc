#ifndef _DEBUG_H_
#define _DEBUG_H_
#define GDEBUG

#ifdef GDEBUG
#define PR_DEBUG()	(cout <<__FILE__<< ":	"<<__LINE__  << "==> " << __FUNCTION__ << endl)
#else
#define PR_DEBUG()	NULL
#endif

#endif

