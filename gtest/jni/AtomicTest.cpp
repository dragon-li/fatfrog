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

#include <vector>
#include <thread>

#undef NAMESPACE
#define NAMESPACE liyl 

typedef struct TmpStruct 
{
    const char* str;
    int         count;
    int         count1;
} TmpStruct;

std::atomic<int> g_guard(0);
volatile int g_guard_l = 0;

TmpStruct gStruct = {0};
NAMESPACE::Mutex gLock;

#define MY_CHECK_NUM 4000
#define MY_READ_STRUCT  4000
#define MY_WRITE_STRUCT 600000
#define MY_THREAD_NUM 100
#define MY_THREAD_GAP_US 000000

void initStruct_org() {
    for (volatile int n = 0; n < MY_WRITE_STRUCT; n+=2) {
            if(g_guard_l == 1)
				continue;
            gStruct.str = "liyl";
            gStruct.count = n;
            gStruct.count1 = n;
            gStruct.count = n+1;
            gStruct.count1 = n+1;


            g_guard_l = 1;
    }


}

void initStruct_atomic() {
    for (volatile int n = 0; n < MY_WRITE_STRUCT; n+=2) {
            if(g_guard.load(std::memory_order_acquire) == 1)
				continue;
            gStruct.str = "liyl";
            gStruct.count = n;
            gStruct.count1 = n;
            gStruct.count = n+1;
            gStruct.count1 = n+1;



            g_guard.store(1,std::memory_order_release);
    }

}

void initStruct_l() {
    for (volatile int n = 0; n < MY_WRITE_STRUCT; n+=2) {
        NAMESPACE::Mutex::Autolock _l(gLock);
        if(g_guard_l == 1)
			continue;
        gStruct.str = "liyl";

		gStruct.count = n;
        gStruct.count1 = n;
        gStruct.count = n+1;
        gStruct.count1 = n+1;


        g_guard_l = 1;
    }


}

void checkStruct_org(TmpStruct& tmpStruct) {
    for (volatile int n = 0; n < MY_CHECK_NUM; ++n) {
        int ready = g_guard_l;
        if(ready) {
            ASSERT_EQ(gStruct.count, gStruct.count1);
			g_guard_l = 0;
        }
    }
}

void checkStruct_atomic(TmpStruct& tmpStruct) {
    for (volatile int n = 0; n < MY_CHECK_NUM; ++n) {
        int ready = g_guard.load(std::memory_order_acquire); //occurence by chance when 300 thread, must occurence when 500 thread 
        if(ready) {
            ASSERT_EQ(gStruct.count, gStruct.count1);
			g_guard.store(0,std::memory_order_release);
        }
    }
}

void checkStruct_l(TmpStruct& tmpStruct) {
    for (volatile int n = 0; n < MY_CHECK_NUM; ++n) {
        NAMESPACE::Mutex::Autolock _l(gLock);
        int ready = g_guard_l;
        if(ready) {
            ASSERT_EQ(gStruct.count, gStruct.count1);
        g_guard_l = 0;
	}
    }

}

void my_testMultThreadAcquire_org() {
    for (volatile int n = 0; n < MY_READ_STRUCT; ++n) {
        TmpStruct tmp;
        checkStruct_org(tmp);
    }
}


void my_testMultThreadAcquire_atomic() {
    for (volatile int n = 0; n < MY_READ_STRUCT; ++n) {
        TmpStruct tmp;
        checkStruct_atomic(tmp);
    }
}
void my_testMultThreadAcquire_l() {
    for (volatile int n = 0; n < MY_READ_STRUCT; ++n) {
        TmpStruct tmp;
        checkStruct_l(tmp);
    }
}

TEST(AtomicTest, TestMultThreadAcquireOrg) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
    for (volatile int n = 0; n < MY_THREAD_NUM; ++n) {
        h.emplace_back(initStruct_org);
        v.emplace_back(my_testMultThreadAcquire_org);
        usleep(MY_THREAD_GAP_US);
    }
    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
}

TEST(AtomicTest, TestMultThreadAcquireLock) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
    for (volatile int n = 0; n < MY_THREAD_NUM; ++n) {
        h.emplace_back(initStruct_l);
        v.emplace_back(my_testMultThreadAcquire_l);
        usleep(MY_THREAD_GAP_US);
    }
    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
}

TEST(AtomicTest, TestMultThreadAcquireAtomic) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
    for (volatile int n = 0; n < MY_THREAD_NUM; ++n) {
        h.emplace_back(initStruct_atomic);
        v.emplace_back(my_testMultThreadAcquire_atomic);
        usleep(MY_THREAD_GAP_US);
    }

    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
}
#undef NAMESPACE
