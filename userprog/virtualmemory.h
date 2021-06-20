#ifndef VIRTUALMEMORY_H
#define VIRTUALMEMORY_H
#include "translate.h"  /* TranslationEntry */
#include "machine.h"    /* pageTable */
#include "sysdep.h"     /* cout, strcmp */
#include "main.h"       /* kernel */

unsigned int PageToOffset(unsigned int Page);
class VirtualMemoryManagement {
    public:
        VirtualMemoryManagement(char *pageReplacementAlgorithm);
        ~VirtualMemoryManagement();
        unsigned int usedFrameNum;
        unsigned int usedPageNum;
        TranslationEntry **frameToPage; /* record which entry use this physical page, swap out use */
        int *lastUsedTick; /* LRU use */
        unsigned int *usedCount; /* LFU use */
        OpenFile *diskMemory;
        void PageFaultHandler(void);
        void SetFIFOPageNumber(unsigned int fifoPageNumber) { this->fifoPageNumber = fifoPageNumber; }
    private:
        unsigned int (VirtualMemoryManagement::*pageReplacement)(void);
        unsigned int FirstInFirstOut(void);
        unsigned int SecondChance(void);
        unsigned int EnhancedSecondChance(void);
        unsigned int LeastRecentlyUsed(void);
        unsigned int LeastFrequentlyUsed(void);
        unsigned int Random(void);
        void SwapPage(TranslationEntry *SwapIn, TranslationEntry *SwapOut);
        unsigned int fifoPageNumber;
};
#endif