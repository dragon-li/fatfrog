/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef LIYL_OMX_MASTER_H_

#define LIYL_OMX_MASTER_H_

#include "api/OMXPluginBase.h"

#include "../foundation/system/core/threads.h"
#include "../foundation/system/core/utils/KeyedVector.h"
#include "../foundation/system/core/utils/List.h"
#include "../foundation/media/include/AString.h"

LIYL_NAMESPACE_START

struct OMXMaster : public OMXPluginBase {
    OMXMaster();
    virtual ~OMXMaster();

    virtual OMX_ERRORTYPE makeComponentInstance(
            const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

    virtual OMX_ERRORTYPE destroyComponentInstance(
            OMX_COMPONENTTYPE *component);

    virtual OMX_ERRORTYPE enumerateComponents(
            OMX_STRING name,
            size_t size,
            OMX_U32 index);

    virtual OMX_ERRORTYPE getRolesOfComponent(
            const char *name,
            Vector<AString> *roles);

private:
    char mProcessName[16];
    Mutex mLock;
    List<OMXPluginBase *> mPlugins;
    KeyedVector<AString, OMXPluginBase *> mPluginByComponentName;
    KeyedVector<OMX_COMPONENTTYPE *, OMXPluginBase *> mPluginByInstance;

    void *mVendorLibHandle;

    void addVendorPlugin();
    void addPlugin(const char *libname);
    void addPlugin(OMXPluginBase *plugin);
    void clearPlugins();

    OMXMaster(const OMXMaster &);
    OMXMaster &operator=(const OMXMaster &);
};

LIYL_NAMESPACE_END

#endif  // LIYL_OMX_MASTER_H_
