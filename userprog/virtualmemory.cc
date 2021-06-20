#include "virtualmemory.h"

unsigned int PageToOffset(unsigned int Page)
{
    return Page << PageMaskBit;
}

VirtualMemoryManagement::VirtualMemoryManagement(char *pageReplacementAlgorithm)
{
    frameToPage = new TranslationEntry*[NumPhysPages];
    lastUsedTick = new int[NumPhysPages];
    usedCount = new unsigned int[NumPhysPages];

    for (unsigned int i = 0; i < NumPhysPages; ++i) {
        frameToPage[i] = NULL;
        lastUsedTick[i] = 0;
        usedCount[i] = 0;
    }

    usedFrameNum = 0;
    usedPageNum = 0;
    fifoPageNumber = 0;
    pageReplacement = NULL;

    /* must to create a empty file as disk, or virtual memory can't work */
    ASSERT(kernel->fileSystem->Create("DiskMemory"));
    diskMemory = kernel->fileSystem->Open("DiskMemory");
    /* page replacement algorithm setting */
    if (pageReplacementAlgorithm) {
        cout << "pageReplacement:";
        if (strcmp(pageReplacementAlgorithm, "fifo") == 0) {
            pageReplacement = &VirtualMemoryManagement::FirstInFirstOut;
            cout << "FirstInFirstOut\n";
        } else if (strcmp(pageReplacementAlgorithm, "sc") == 0) {
            pageReplacement = &VirtualMemoryManagement::SecondChance;
            cout << "SecondChance\n";
        } else if (strcmp(pageReplacementAlgorithm, "esc") == 0) {
            pageReplacement = &VirtualMemoryManagement::EnhancedSecondChance;
            cout << "EnhancedSecondChance\n";
        } else if (strcmp(pageReplacementAlgorithm, "lru") == 0) {
            pageReplacement = &VirtualMemoryManagement::LeastRecentlyUsed;
            cout << "LeastRecentlyUsed\n";
        } else if (strcmp(pageReplacementAlgorithm, "lfu") == 0) {
            pageReplacement = &VirtualMemoryManagement::LeastFrequentlyUsed;
            cout << "LeastFrequentlyUsed\n";
        } else if (strcmp(pageReplacementAlgorithm, "rd") == 0) {
            pageReplacement = &VirtualMemoryManagement::Random;
            cout << "Random\n";
        }
    }

    if (!pageReplacement) {
        pageReplacement = &VirtualMemoryManagement::Random;
        cout << "pageReplacement not setting, default is Random\n";
    }
}

VirtualMemoryManagement::~VirtualMemoryManagement()
{
    kernel->fileSystem->Remove("DiskMemory");
    delete [] frameToPage;
    delete [] lastUsedTick;
    delete [] usedCount;
}

unsigned int VirtualMemoryManagement::FirstInFirstOut(void)
{
    if (fifoPageNumber == NumPhysPages)
        fifoPageNumber -= NumPhysPages;
    return fifoPageNumber++;
}

unsigned int VirtualMemoryManagement::SecondChance(void)
{
    while (1) {
        int VictimPage = FirstInFirstOut();
        TranslationEntry* entry = frameToPage[VictimPage];
        if (!entry->use)
            return VictimPage;
        else
            entry->use = false;
    }
}

unsigned int VirtualMemoryManagement::EnhancedSecondChance(void)
{
    bool Find[4] = {false};
    int FirstFindPage[4] = {-1, -1, -1, -1};
    for (unsigned int i = 0; i < NumPhysPages; ++i) {
        int VictimPage = FirstInFirstOut();
        TranslationEntry* entry = frameToPage[VictimPage];
        int property = 2 * entry->use + entry->dirty;
        switch (property) {
            case 3:
            case 2:
                entry->use = false;
            case 1:
                if (!Find[property]) {
                    Find[property] = true;
                    FirstFindPage[property] = VictimPage;
                }
                break;
            case 0:
                return VictimPage;
                break;
        }
    }

    for (unsigned int i = 1; i < 4; ++i) {
        if (FirstFindPage[i] != -1) {
            SetFIFOPageNumber(FirstFindPage[i] + 1);
            return FirstFindPage[i];
        }
    }

    /* should not be here */
    ASSERTNOTREACHED();
    return 0; 
}

unsigned int VirtualMemoryManagement::LeastRecentlyUsed(void)
{
    int minTick = 0x7fffffff; /* int maximum */
    unsigned int VictimPage = -1;
    for (unsigned int i = 0; i < NumPhysPages; ++i) {
        if (lastUsedTick[i] < minTick) {
            minTick = lastUsedTick[i];
            VictimPage = i;
        }
    }
    return VictimPage;
}

unsigned int VirtualMemoryManagement::LeastFrequentlyUsed(void)
{
    unsigned int minCount = 0xffffffff; /* int maximum */
    unsigned int VictimPage = -1;
    for (unsigned int i = 0; i < NumPhysPages; ++i) {
        if (usedCount[i] < minCount) {
            minCount = usedCount[i];
            VictimPage = i;
        }
    }
    return VictimPage;
}

unsigned int VirtualMemoryManagement::Random(void)
{
    static unsigned int Mask = NumPhysPages - 1;
    return kernel->stats->totalTicks & Mask;
}

void VirtualMemoryManagement::SwapPage(TranslationEntry *SwapIn, TranslationEntry *SwapOut)
{
    unsigned int FrameNumber = SwapOut->physicalPage;
    char *mainMemory = kernel->machine->mainMemory;
    unsigned int mainOffset = PageToOffset(FrameNumber);
    unsigned int diskOffset;
    /* write to disk if dirty */
    if (SwapOut->dirty && SwapOut->inDisk) {
        diskOffset = PageToOffset(SwapOut->virtualPage);
        diskMemory->WriteAt(mainMemory + mainOffset, PageSize, diskOffset);
    /* never exist in disk */
    } else if (!SwapOut->inDisk) {
        SwapOut->virtualPage = usedPageNum++;
        diskOffset = PageToOffset(SwapOut->virtualPage);
        SwapOut->inDisk = true;
        diskMemory->WriteAt(mainMemory + mainOffset, PageSize, diskOffset);
        //printf("Swap Frame ID %d:mainAddr:0x%x to Page ID:%d:diskAddr:0x%x\n", physicalPageNumber, mainOffset, SwapOut->virtualPage, diskOffset);
    }
    /* write to main memory */
    diskOffset = PageToOffset(SwapIn->virtualPage);
    diskMemory->ReadAt(mainMemory + mainOffset, PageSize, diskOffset);
    //printf("Swap Page ID %d:diskAddr:0x%x to Frame ID:%d:mainAddr:0x%x\n", SwapIn->virtualPage, diskOffset, physicalPageNumber, mainOffset);

    /* update swap out page info */
    SwapOut->valid = false;
    /* update swap in page info */
    SwapIn->physicalPage = FrameNumber;
    SwapIn->valid = true;
    SwapIn->dirty = false;
    SwapIn->use = false;
    /* update frameToPage value to new User */
    frameToPage[FrameNumber] = SwapIn;
}

void VirtualMemoryManagement::PageFaultHandler(void)
{
    TranslationEntry *SwapIn, *SwapOut;
    int FaultAddr = kernel->machine->ReadRegister(BadVAddrReg);
    int PageNumber = FaultAddr >> PageMaskBit;
    int FrameNumber = (this->*pageReplacement)(); /* cpp pointer to member function use */

	SwapIn = (kernel->machine->pageTable + PageNumber);
	SwapOut = frameToPage[FrameNumber];
    /* because we have prepage mainMemory mechanism, so pagefault occur always need swap page */
    SwapPage(SwapIn, SwapOut);
}