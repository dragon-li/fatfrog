/*************************************************************************
  > File Name: AtomicTest.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年10月27日 星期五 19时53分13秒
 ************************************************************************/

#define __STDC_LIMIT_MACROS

#include <gtest/gtest.h>


#include "../../jni/foundation/system/core/utils/Log.h"
#include "../../jni/foundation/system/core/utils/Mutex.h"
#include "../../jni/foundation/system/android/atomic-arm.h"

#include <vector>
#include <thread>

#undef NAMESPACE
#define NAMESPACE liyl 

typedef struct TmpStruct_ARM 
{
    const char* str;
    int         count;
    int         count1;
} TmpStruct_ARM;

volatile int g_guard_ARM = 1;
volatile int g_guard_l_ARM = 1;

TmpStruct_ARM gStruct_ARM = {0};
NAMESPACE::Mutex gLock_ARM;

#define MY_CHECK_NUM_ARM 4000
#define MY_READ_STRUCT_ARM  4000
#define MY_WRITE_STRUCT_ARM 80000
#define MY_THREAD_NUM_ARM 1
#define MY_THREAD_GAP_US_ARM 000000

void initStruct_org_ARM() {
    for (volatile int n = 0; n < MY_WRITE_STRUCT_ARM; ++n) {
        g_guard_l_ARM = 0;
        gStruct_ARM.str = "liyl";
        gStruct_ARM.count = n;
        gStruct_ARM.count1 = n;

        g_guard_l_ARM = 1;
    }


}

void initStruct_atomic_ARM() {
    for (volatile int n = 0; n < MY_WRITE_STRUCT_ARM; ++n) {
    	do{
            //std::cout << "g_guard_ARM = "<<g_guard_ARM<< '\n';
		}while(!(liyl_atomic_acquire_cas(1,0,&g_guard_ARM) == 0)); 

        gStruct_ARM.str = "liyl";
        gStruct_ARM.count = n;
        gStruct_ARM.count1 = n;

		do{
            //std::cout << "g_guard_ARM = "<<g_guard_ARM<< '\n';
		}while(!(liyl_atomic_release_cas(0,1,&g_guard_ARM) == 0)); 
    }


}

void initStruct_l_ARM() {
    for (volatile int n = 0; n < MY_WRITE_STRUCT_ARM; ++n) {
        NAMESPACE::Mutex::Autolock _l(gLock_ARM);
        g_guard_l_ARM = 0;
        gStruct_ARM.str = "liyl";
        gStruct_ARM.count = n;
        gStruct_ARM.count1 = n;

        g_guard_l_ARM = 1;
    }


}

void checkStruct_org_ARM(TmpStruct_ARM& tmpStruct) {
    for (volatile int n = 0; n < MY_CHECK_NUM_ARM; ++n) {
        while (!g_guard_l_ARM);
        ASSERT_EQ(gStruct_ARM.count, gStruct_ARM.count1);
    }
}

void checkStruct_atomic_ARM(TmpStruct_ARM& tmpStruct) {
    for (volatile int n = 0; n < MY_CHECK_NUM_ARM; ++n) {
		do {
            //std::cout << "g_guard_ARM = "<<g_guard_ARM<< '\n';
        }
		while(!(liyl_atomic_acquire_cas(1,2,&g_guard_ARM) == 0));
        ASSERT_EQ(gStruct_ARM.count, gStruct_ARM.count1);
		do {

            //std::cout << "g_guard_ARM = "<<g_guard_ARM<< '\n';
        }
		while(!(liyl_atomic_release_cas(2,1,&g_guard_ARM) == 0));

    }
}

void checkStruct_l_ARM(TmpStruct_ARM& tmpStruct) {
    for (volatile int n = 0; n < MY_CHECK_NUM_ARM; ++n) {
        NAMESPACE::Mutex::Autolock _l(gLock_ARM);
        while (!g_guard_l_ARM);
        ASSERT_EQ(gStruct_ARM.count, gStruct_ARM.count1);
    }

}

void my_testMultThreadAcquire_org_ARM() {
    for (volatile int n = 0; n < MY_READ_STRUCT_ARM; ++n) {
        TmpStruct_ARM tmp;
        checkStruct_org_ARM(tmp);
    }
}


void my_testMultThreadAcquire_atomic_ARM() {
    for (volatile int n = 0; n < MY_READ_STRUCT_ARM; ++n) {
        TmpStruct_ARM tmp;
        checkStruct_atomic_ARM(tmp);
    }
}
void my_testMultThreadAcquire_l_ARM() {
    for (volatile int n = 0; n < MY_READ_STRUCT_ARM; ++n) {
        TmpStruct_ARM tmp;
        checkStruct_l_ARM(tmp);
    }
}

TEST(AtomicTest_ARM, TestMultThreadAcquireOrg_ARM) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
    for (volatile int n = 0; n < MY_THREAD_NUM_ARM; ++n) {
        h.emplace_back(initStruct_org_ARM);
        v.emplace_back(my_testMultThreadAcquire_org_ARM);
        usleep(MY_THREAD_GAP_US_ARM);
    }
    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
}

TEST(AtomicTest_ARM, TestMultThreadAcquireLock_ARM) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
    for (volatile int n = 0; n < MY_THREAD_NUM_ARM; ++n) {
        h.emplace_back(initStruct_l_ARM);
        v.emplace_back(my_testMultThreadAcquire_l_ARM);
        usleep(MY_THREAD_GAP_US_ARM);
    }
    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
}

TEST(AtomicTest_ARM, TestMultThreadAcquireAtomic_ARM) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
    for (volatile int n = 0; n < MY_THREAD_NUM_ARM; ++n) {
        h.emplace_back(initStruct_atomic_ARM);
        v.emplace_back(my_testMultThreadAcquire_atomic_ARM);
        usleep(MY_THREAD_GAP_US_ARM);
    }

    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
}
#undef NAMESPACE
