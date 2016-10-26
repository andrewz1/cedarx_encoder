#ifndef _EXPORT_H
#define _EXPORT_H
#if __GNUC__ >= 4
	#define EXPORT __attribute__ ((visibility ("default")))
#else
    #define EXPORT
#endif
#endif /* _EXPORT_H */
