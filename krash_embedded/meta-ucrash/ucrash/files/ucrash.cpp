/********1*********2*********3****** ***4*********5**********6*********7*********/
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

#define UserMode
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

pthread_mutex_t gmtx;


void* crashFaultThread(void* pvoid)
{
crash_private *priv = (crash_private *)pvoid;
int arg = priv->arg;
long int rc = 0;

    std::cout << "CrashFaultThread...  arg=" << arg << std::endl;
    
    switch (arg)
	{
        case 1:
            pthread_mutex_lock(priv->pmtx);
            break;   
         
        default:;
            rc = -EINVAL;
        }
              
    std::cout << "CrashFaultThread...  rc=" << rc << std::endl;
    
    pthread_exit((void *)rc);
}

void freeSomething(void *ptr)
{
    std::cout << "ucrash... freeSomething: ptr=";
    std::cout << std::hex << ptr << std::endl;
    free(ptr);
}

void reuseFree(void *ptr)
{
    std::cout << "ucrash... reuseFree: ptr=";
    std::cout << std::hex << ptr << std::endl;
    freeSomething(ptr);
    freeSomething(ptr);
    memset(ptr, 0x78, 100);
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

void eventHang(void)
{
    int io_f = 0;
    std::time_t now = std::time({});
    pthread_t Thread1;
    pthread_t Thread2;
    crash_private cpriv1;
    crash_private cpriv2;
    int rc = 0;
                         
    std::cout << "ucrash: Event Hang... time=" << now << std::endl;
    
    pthread_mutex_init(&gmtx, NULL);
    //pthread_mutex_lock(&gmtx);
    cpriv1.arg = 1;
    cpriv1.pmtx = &gmtx;
    cpriv1.nMinor=1;
    rc = pthread_create(&Thread1, NULL, crashFaultThread, &cpriv1);
    if (rc == 0)
        {
        cpriv2.arg = 1;
        cpriv2.pmtx = &gmtx;
        cpriv2.nMinor=2;
        rc = pthread_create(&Thread2, NULL, crashFaultThread, &cpriv2);
        }
        
    if (rc != 0)
        std::cout << "ucrash: Event Hang ... rc=" << rc << std::endl;
 
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
        //schedule();
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

void trashStack(void)
{
    char stackData[100];
    char* stackPtr = stackData;

    std::cout << "trashStack()... ";
    stackPtr -= 50;
    memset(stackData, 0x41, 100); 
    memset(stackPtr, 0xff, 100);
    
    std::cout <<  "trashStack() exit" << std::endl;
   
    return;
}

void overrunStack(void)
{
    int i, j;
    char stackData[2020];
    
    std::cout << "overrunStack() ... ";
       for (j=0; j < 100000; j++)
         {
         i = 2020 - j;
         stackData[i] = (char)i; //(void*)(uintptr_t)i;
         }
    
    std::cout <<  "recursive call overrunStack()" << std::endl;
    overrunStack();
    std::cout <<  "overrunStack() exit" << std::endl;
        
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

void display_help(void)
{
    std::cout << std::endl 
              << "Command line arguments: ucrash -u? *OR* ucrash -k?" << std::endl
              << "where one of  Z 0 N R F P X ... H D W L M V T  selects... " << std::endl << std::endl
              << "Z)ero-divide 0)null-pntr i(N)vld-pntr R)eusefree bad(F)ree bad(P)age write-(X)ecutable" << std::endl
              << "... " << std::endl
              << "H)ardrun D)eadlock W)atchdog L)eak-mem no-(M)emory stack: o(V)errun T)rashed" << std::endl
              << std::endl << std::flush;
}

 
int main(int argc, char* argv[])
{
    int crashnum = eUndef;
    int krashnum = eUndef;
    int rc = 0;
    void* ptr = nullptr;
    char crash_list[] = Crash_List;
    Crash_Funxns eCrashFunxn = eUndef;
    char* pc = NULL;
    long int ndx = -1;
 
    std::cout << "ucrash app started... ";
    std::cout << "Process Id: " << ::getpid();

    /* Throw SIGABRT */
    if (argc > 1)
        {
        argv[1][1] = tolower(argv[1][1]);
        argv[1][2] = tolower(argv[1][2]);
        pc = strchr(crash_list, argv[1][2]);
        if (pc == NULL)
            {
            display_help();
            rc = -EINVAL;
            exit(rc);
            }
                
        ndx = (long int)pc - (long int)crash_list;
        eCrashFunxn = (Crash_Funxns)ndx;
                
        if (memcmp(argv[1], "-u", 2) == 0)
            {crashnum = eCrashFunxn;} 

        if (memcmp(argv[1], "-k", 2) == 0)
            {krashnum = eCrashFunxn;}
        }

    std::cout << "  U-crash=" << crashnum << ", K-crash=" << krashnum << std::endl;
    if ((int)krashnum != eUndef)
        {
        int fd = open(KRASH_DEVICE_PATH, (O_RDWR));
        if (fd < 0)
            {
            std::cout << std::endl << "kernel module open failure: errno=" << errno << std::endl;
            std::cout << "insmod /lib/modules/<linuxver>/extra/krash.ko" << std::endl;
            rc = errno;
            exit(rc);
            }

        rc = ioctl(fd, KRASH_DEV_IOC_KRASH_NUM, krashnum);
        if (rc < 0)
            {
            rc = errno;
            std::cout << "kernel ioctl rejected: rc=" << rc << std::endl;
            }
            
        close(fd);
        exit(rc);
        }
    
    switch (crashnum)
        {
        case eDivZero: //divided by zero
            zeroDivide(); 
            break;

        case eNullPntr: //nullptr
            {
            freeSomething(NULL);
            }
            break;

        case eInvalidPntr: //invalid ptr
            {
            ptr = &crashnum;
            freeSomething(ptr);
            }
            break;

        case eReuseFree: //double free
            {
            ptr = malloc(0x1000);
            reuseFree(ptr);
            }
            break;
            
        case eBadFree: //double free
            {
            ptr = (void *)-1;
            freeSomething(ptr);
            }
            break; 
                       
        /*case eBadPage: 
            {
            ptr = malloc(0x1000);
            freeSomething(ptr);
            freeSomething((char *)(long int)ptr-4096);
            }
            break; */            

        case eXecMem: //reference executable memory
            writeExecMemory();
            break; 

        case eHardRun:
            hardRun();
            break; 
            
        case eWatchdog:
            //pthread_mutex_lock(&gmtx);
            eventHang();
            break; 
            
        case eLeakMem:
            leakMemory();
            break; 
            
        case eNoMem:
            exhaustMemory();
            break; 
 
        case eStkOvr:
            overrunStack();
            break;
        
        case eStkTrash:
            trashStack();
            break;

        case eBadPage: 
        case eDeadLock:
            rc = -ENOSYS;
            break;

/* */
        default:
            display_help();
            rc = -EINVAL;
            break;
        }
   
    std::cout << "ucrash exit=" << rc << std::endl << std::flush;
    exit(rc);
}
