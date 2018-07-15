/*
* Copyright (C) 2014 MediaTek Inc.
* Modification based on code covered by the mentioned copyright
* and/or permission notice(s).
*/
/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIYL_MEDIA_CODEC_LIST_H_

#define LIYL_MEDIA_CODEC_LIST_H_

#include "../../foundation/media/include/ABase.h"
#include "../../foundation/media/include/AString.h"
#include "../api/IMediaCodecList.h"
#include "../../omx/api/IOMX.h"
#include "MediaCodecInfo.h"

#include "../../foundation/system/core/types.h"
#include "../../foundation/system/core/utils/Errors.h"
#include "../../foundation/system/core/utils/KeyedVector.h"
#include "../../foundation/system/core/utils/Vector.h"
#include "../../foundation/system/core/utils/StrongPointer.h"

LIYL_NAMESPACE_START


struct AMessage;

class MediaCodecList : public IMediaCodecList {
public:

    virtual ssize_t findCodecByType(
            const char *type, bool encoder, size_t startIndex = 0) const;

    virtual ssize_t findCodecByName(const char *name) const;

    virtual size_t countCodecs() const;

    virtual sp<MediaCodecInfo> getCodecInfo(size_t index) const {
        if (index >= mCodecInfos.size()) {
            LLOGE("b/24445127");
            return NULL;
        }
        return mCodecInfos.itemAt(index);
    }

    virtual const sp<AMessage> getGlobalSettings() const;

    // to be used by MediaPlayerService alone
    static sp<IMediaCodecList> getLocalInstance();
	// release global varying
    static void destoryLocalInstance();

    // only to be used by getLocalInstance
    static void *profilerThreadWrapper(void * /*arg*/);

    // only to be used by MediaPlayerService
    void parseTopLevelXMLFile(const char *path, bool ignore_errors = false);

    static void findMatchingCodecs(
                    const char *mime,
                    bool createEncoder,
                    uint32_t flags,
                    Vector<AString> *matching);
    static uint32_t getQuirksFor(const char *mComponentName);
private:

    enum Section {
        SECTION_TOPLEVEL,
        SECTION_SETTINGS,
        SECTION_DECODERS,
        SECTION_DECODER,
        SECTION_DECODER_TYPE,
        SECTION_ENCODERS,
        SECTION_ENCODER,
        SECTION_ENCODER_TYPE,
        SECTION_INCLUDE,
    };

    static sp<IMediaCodecList> sCodecList;

    status_t mInitCheck;
    Section mCurrentSection;
    bool mUpdate;
    Vector<Section> mPastSections;
    int32_t mDepth;
    AString mHrefBase;

    sp<AMessage> mGlobalSettings;

    Vector<sp<MediaCodecInfo> > mCodecInfos;
    sp<MediaCodecInfo> mCurrentInfo;
    sp<IOMX> mOMX;

    MediaCodecList();
    ~MediaCodecList();

    status_t initCheck() const;
    void parseXMLFile(const char *path);

    static void StartElementHandlerWrapper(
            void *me, const char *name, const char **attrs);

    static void EndElementHandlerWrapper(void *me, const char *name);

    void startElementHandler(const char *name, const char **attrs);
    void endElementHandler(const char *name);

    status_t addSettingFromAttributes(const char **attrs);
    status_t addMediaCodecFromAttributes(bool encoder, const char **attrs);
    void addMediaCodec(bool encoder, const char *name, const char *type = NULL);

    void setCurrentCodecInfo(bool encoder, const char *name, const char *type);

    status_t addQuirk(const char **attrs);
    status_t addTypeFromAttributes(const char **attrs);
    status_t addLimit(const char **attrs);
    status_t addFeature(const char **attrs);
    void addType(const char *name);

    //liyl add
    status_t QuerryCapabilities( const AString &name, const AString &mime, sp<MediaCodecInfo::Capabilities> *caps);
    status_t initializeCapabilities(const char *type);

    DISALLOW_EVIL_CONSTRUCTORS(MediaCodecList);
};

LIYL_NAMESPACE_END

#endif  // MEDIA_CODEC_LIST_H_

