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
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
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

#include "../../jni/foundation/system/core/utils/Log.h"
#include "../../jni/foundation/system/core/utils/Thread.h"
#undef NAMESPACE
#define NAMESPACE liyl 

class ThreadA :virtual public NAMESPACE::Thread {
	
	 
private: 
	   
	   void printTid() {
		   ASSERT_NE(-1,getTid());
	   }
	   virtual bool threadLoop() {
		   printTid();
		   return false;
	   }
private:
};

TEST(ThreadTest, TestGetTid) {
	NAMESPACE::sp<ThreadA> aa = new ThreadA();
	 aa->run("liyl");
	 aa->join();


}

#undef NAMESPACE
