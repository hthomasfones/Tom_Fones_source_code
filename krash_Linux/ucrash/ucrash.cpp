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
#include <ctime>
#include <iostream>
#include <locale>
#include <string>
#include <cstring>
#include <vector>
#include <iterator>

#define SLEEP_TIME 700
#define KRASH_DEVICE_PATH "/dev/krashdev"

void freeSomething(void *ptr)
{
    std::cout << "ucrash... freeSomething ";
    std::cout << std::hex << ptr << std::endl;
    free(ptr);
}

int zeroDivide()
{   
    int rc = 0;
    int five = 5;
    int zero = 0;

    std::cout << "ucrash... Divide by zero " << std::endl;
    rc = five/zero;

    return rc;
}

void hardRun(void)
{
    unsigned int cpuid = -1;
    int rc = getcpu(&cpuid, NULL);
    std::time_t now = std::time({});
    std::vector<int> Now(20);
    //std::strftime(std::data(Now), 20,
     //                 "%FT%TZ", std::gmtime(&now));
    std::cout << "ucrash... Hard run cpu=" << cpuid << " time=" << now << std::endl;
    int X = 0;
    while (true)
        {
        X += 5;
        X -= 5;
        }

    return;
}

void exhaustMemory(void)
{
    std::cout << "ucrash... Exhaust memory" << std::endl;
    void* ptr = NULL;
    while (true)
        {
        ptr = malloc(0x4000);
        if (ptr==NULL)
            break;
        schedule();
        }

    ptr = malloc(0x4000);
    return;
}

void leakMemory(void)
{
    std::cout << "ucrash... Leak memory" << std::endl;
    void* ptr = NULL;
    int n = 0;

    for (n=0; n < 12; n++)
        {
        ptr = malloc(0x50000);
        }
    free(ptr);

    return;
}

void writeExecMemory(void)
{
    void* ptr = (void *)&writeExecMemory;
    std::cout << "ucrash... Write executable memory ";
    std::cout << std::hex << ptr << std::endl;
    memset(ptr, 0x00, 10);

    return;
}

int main(int argc, char* argv[])
{
    int  crashnum = -1;
    int  krashnum = -1;
    int rc = 0;
    void* ptr = nullptr;

    std::cout << "ucrash app started... ";
    std::cout << "Process Id: " << ::getpid();

    /* Throw SIGABRT */
    if (argc > 1)
        {
        argv[1][1] = tolower(argv[1][1]);
        if (memcmp(argv[1], "-u", 2) == 0)
            crashnum = (int)(argv[1][2] - 0x30);

        if (memcmp(argv[1], "-k", 2) == 0)
            {
            krashnum = (int)(argv[1][2] - 0x30);
            crashnum = -1;
            }
        }

    std::cout << "  crashnum=" << crashnum << ", krashnum=" << krashnum << std::endl;
    switch (crashnum)
        {
        case 0: //divided by zero
            zeroDivide(); 
            break;

        case 1: //nullptr
            {
            freeSomething(NULL);
            }
            break;

        case 2: //invalid ptr
            {
            ptr = &crashnum;
            freeSomething(ptr);
            }
            break;

        case 3: //double free
            {
            ptr = malloc(0x1000);
            freeSomething(ptr);
            freeSomething(ptr);
            }
            break;

        case 4: //reference executable memory
            writeExecMemory();
            break; 

        case 5:
            hardRun();
            break; 

        case 6:
            exhaustMemory();
            break; 

        case 7:
            leakMemory();
            break; 
/*                                                                  */
        /*case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18: */
        case -1:
            {
            int fd = open(KRASH_DEVICE_PATH, (O_RDWR));
            if (fd < 0)
                {
                rc = errno;
                break;
                }

            rc = ioctl(fd, KRASH_DEV_IOC_KRASH_NUM, krashnum);
            close(fd);
            }
            break;

        default:
            std::cout << "ucrash -uX  (X = 0..7)" << std::endl
                      << "ucrash -kX  (X = 0..9)" << std::endl
            << "0..9: N/0, NullPntr, InvalidPntr, DoubleFree, ExecMemory" << std::endl
            << "      HardRun, ExhaustMemory, LeakMemory, NotReady NotReady" 
            << std::endl << std::flush;
            break;
        }
   
    std::cout << "ucrash exit=" << rc << std::endl << std::flush;
    exit(rc);
}
