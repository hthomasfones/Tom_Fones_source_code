/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Krash                                                       */
/*  COPYRIGHT    : (c) 2023 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by Dual/GPL license to the Linux open source   */ 
/* community                                                                   */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe files   : ucrash.o                                                     */ 
/*                                                                             */
/*  Module NAME : ucrash.cpp                                                   */
/*                                                                             */
/*  DESCRIPTION : Properties & Definitions for the user-mode crash program     */
/*                                                                             */
/*  MODULE_AUTHOR("HTF Consulting");                                           */
/*  MODULE_LICENSE("Dual/GPL");                                                */
/*                                                                             */
/* The source code in this file can be freely used, adapted, and redistributed */
/* in source or binary form, so long as an acknowledgment appears in derived   */
/* source files.  The citation should state that the source code comes from    */
/* a set of source files developed by HTF Consulting                           */
/* http://www.htfconsulting.com                                                */
/*                                                                             */
/* No warranty is attached.                                                    */
/* HTF Consulting takes no responsibility for errors or fitness of use         */
/*                                                                             */
/*                                                                             */
/* $Id: ucrash.cpp, v 1.0 2023/01/01 00:00:00 htf $                            */
/*                                                                             */
/*******************************************************************************/

#include "ucrash.h"

#define SLEEP_TIME 700
#define KRASH_DEVICE_PATH "/dev/krashdev"

void freeSomething(void *ptr)
{
    free(ptr);
}

int zeroDivide()
{
    int nDivider = 5;
    int nRes = 0;
    while(nDivider > 0)
        {
        nDivider--;
        nRes = 5 / nDivider;
        }
    return nRes;
}

int main(int argc, char* argv[])
{
    int  crashnum = 0;
    int  krashnum = 0;
    int rc = 0;

    std::cout << "ucrash app started..." << std::endl;
    std::cout << "Process Id: " << ::getpid() << std::endl;

    /* Throw SIGABRT */
    if (argc > 1)
        {
        if (memcmp(argv[1], "-c", 2) == 0)
            crashnum = (int)(argv[1][2] - 0x30);

        if (memcmp(argv[1], "-k", 2) == 0)
            crashnum = (int)((argv[1][2] - 0x30) + 10);
        }

    std::cout << "crashnum = " << crashnum << std::endl;
    switch (crashnum)
        {
        case 1: //divided by zero
            zeroDivide(); 
            break;

        case 2: //nullptr
            {
            void* ptr = nullptr;
            freeSomething(ptr);
            }
            break;

        case 3: //invalid ptr
            {
            void* ptr = &crashnum;
            freeSomething(ptr);
            }
            break;

        case 4: //double free
            {
            void* ptr = malloc(0x1000);
            freeSomething(ptr);
            freeSomething(ptr);
            }
            break;

        case 5: //reference executable memory
            {
            void* ptr = (void *)freeSomething; 
            memset(ptr, 0x00, 10);
            }
            break;

        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            krashnum = crashnum - 10;
            {
            int fd = open(KRASH_DEVICE_PATH, (O_RDWR));
            if (fd < 0)
                {
                rc = errno;
                break;
                }

            rc = ioctl(fd, KRASH_DEV_IOC_KRASH_NUM, krashnum);
            }
            break;

        default:
            std::cout << "ucrash -cX  (X = 1..5)" << std::endl
                      << "ucrash -kX  (X = 1..5)" << std::endl
            << "X/0, nullptr, invalid pointer, double free, reference exec memory"
            << std::endl << std::flush;
            break;
        }
   
    std::cout << "ucrash - exit=" << rc << std::endl << std::flush;
    exit(rc);
}
