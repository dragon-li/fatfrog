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
#include "../../jni/foundation/system/core/utils/SharedBuffer.h"
#undef NAMESPACE
#define NAMESPACE liyl 

TEST(SharedBufferTest, TestAlloc) {
  LLOGD("NAMESPACE add %s",__func__);
  EXPECT_DEATH(NAMESPACE::SharedBuffer::alloc(SIZE_MAX), "");
  EXPECT_DEATH(NAMESPACE::SharedBuffer::alloc(SIZE_MAX - sizeof(NAMESPACE::SharedBuffer)), "");

  // Make sure we don't die here.
  // Check that null is returned, as we are asking for the whole address space.
  NAMESPACE::SharedBuffer* buf =
      NAMESPACE::SharedBuffer::alloc(SIZE_MAX - sizeof(NAMESPACE::SharedBuffer) - 1);
  ASSERT_EQ(nullptr, buf);

  buf = NAMESPACE::SharedBuffer::alloc(0);
  ASSERT_NE(nullptr, buf);
  ASSERT_EQ(0U, buf->size());
  buf->release();
}

TEST(SharedBufferTest, TestEditResize) {
  LLOGD("NAMESPACE add %s",__func__);
  NAMESPACE::SharedBuffer* buf = NAMESPACE::SharedBuffer::alloc(10);
  EXPECT_DEATH(buf->editResize(SIZE_MAX - sizeof(NAMESPACE::SharedBuffer)), "");
  buf = NAMESPACE::SharedBuffer::alloc(10);
  EXPECT_DEATH(buf->editResize(SIZE_MAX), "");

  buf = NAMESPACE::SharedBuffer::alloc(10);
  // Make sure we don't die here.
  // Check that null is returned, as we are asking for the whole address space.
  buf = buf->editResize(SIZE_MAX - sizeof(NAMESPACE::SharedBuffer) - 1);
  ASSERT_EQ(nullptr, buf);

  buf = NAMESPACE::SharedBuffer::alloc(10);
  buf = buf->editResize(0);
  ASSERT_EQ(0U, buf->size());
  buf->release();
}
#undef NAMESPACE
