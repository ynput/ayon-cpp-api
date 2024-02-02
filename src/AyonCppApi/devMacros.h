#ifndef DEV_MACRO_H
#define DEV_MACRO_H

#if JTRACE == 1
    #define PerfTimer(x) InstrumentationTimer timer(x);
#else
    #define PerfTimer(x)
#endif

#endif   // DEV_MACRO_H
