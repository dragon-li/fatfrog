/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef LIYL_SOFT_OMX_PLUGIN_H_

#define LIYL_SOFT_OMX_PLUGIN_H_

#include "../foundation/media/include/ABase.h"
#include "../foundation/media/include/AString.h"
#include "api/OMXPluginBase.h"

LIYL_NAMESPACE_START

struct SoftOMXPlugin : public OMXPluginBase {
    SoftOMXPlugin();

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
    DISALLOW_EVIL_CONSTRUCTORS(SoftOMXPlugin);
};

LIYL_NAMESPACE_END

#endif  // LIYL_SOFT_OMX_PLUGIN_H_
