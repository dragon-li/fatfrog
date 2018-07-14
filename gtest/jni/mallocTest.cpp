/*************************************************************************
	> File Name: ThreadsTest.cpp
	> Author: liyunlong
	> Mail: liyunlong_88@126.com 
	> Created Time: 2017年11月02日 星期四 19时45分27秒
 ************************************************************************/


/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define __STDC_LIMIT_MACROS

#include <gtest/gtest.h>

#include <memory>
#include <stdint.h>
#include <vector>
#include <thread>


#include "../../jni/foundation/system/core/utils/Log.h"
#undef NAMESPACE
#define NAMESPACE liyl 
#define MY_THREAD_GAP_US  200000
#define MY_MALLOC_SIZE 100000
#define MY_THREAD_NUM 100

void mallocObject(void** memPtr) {
	void** mem_ptr = memPtr;
	*mem_ptr = malloc(MY_MALLOC_SIZE*(((int)memPtr)&0xFF));
	memset(*mem_ptr,'0',MY_MALLOC_SIZE*(((int)memPtr)&0xFF));
	//LLOGD("alloc mem = %p",*mem_ptr);

}
void freeObject(void* mem) {
	if(mem != NULL){
	    //LLOGD("free mem = %p",mem);
		free(mem);
		mem = NULL;
	}
}

TEST(MallocTest, TestMalloc) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
	void* mem[MY_THREAD_NUM] = {NULL,};
    for (volatile int n = 0; n < MY_THREAD_NUM; ++n) {
        h.emplace_back(mallocObject,&(mem[n]));
        usleep(MY_THREAD_GAP_US);
        v.emplace_back(freeObject,mem[n]);
    }

    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
	for (void* tmp : mem) {
		ASSERT_NE((int)tmp , NULL);
	}

}

TEST(MallocTest, TestMalloc1) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
	void* mem[MY_THREAD_NUM] = {NULL,};
    for (volatile int n = 0; n < MY_THREAD_NUM; ++n) {
        mallocObject(&(mem[n]));
        freeObject(mem[n]);
    }

    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
	for (void* tmp : mem) {
		ASSERT_NE((int)tmp , NULL);
	}

}

TEST(MallocTest, TestMalloc2) {
    std::vector<std::thread> v;
    std::vector<std::thread> h;
	void* mem[MY_THREAD_NUM] = {NULL,};
    for (volatile int n = 0; n < MY_THREAD_NUM; ++n) {
        mallocObject(&(mem[n]));
    }

    usleep(MY_THREAD_GAP_US*10);

    for (volatile int n = 0; n < MY_THREAD_NUM-1; ++n) {
        freeObject(mem[n]);
    }



    for (auto& r : v) {
        r.join();
    }
    for (auto& w : h) {
        w.join();
    }
	for (void* tmp : mem) {
		ASSERT_NE((int)tmp , NULL);
	}

}

TEST(MallocTest, TestMalloc3) {
	void* mem_ptr = malloc(MY_MALLOC_SIZE);
	memset(mem_ptr,'0',MY_MALLOC_SIZE+1048+33);
	if(mem_ptr != NULL) {
		free(mem_ptr);
	}
	
}

TEST(MallocTest, TestMalloc4) {
	void* mem_ptr = malloc(MY_MALLOC_SIZE);
	memset(mem_ptr,'0',MY_MALLOC_SIZE+2048+33);
	if(mem_ptr != NULL) {
		free(mem_ptr);
	}
	
}
TEST(MallocTest, TestMalloc5) {
	void* mem_ptr = malloc(MY_MALLOC_SIZE);
	memset(&((uint8_t*)mem_ptr)[MY_MALLOC_SIZE+128],'0',128);
	if(mem_ptr != NULL) {
		free(mem_ptr);
	}
	
}
#undef NAMESPACE
