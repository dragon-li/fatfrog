/*
 * Copyright 2014, The Android Open Source Project
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

#ifndef LIYL_MEDIA_CODEC_INFO_H_

#define LIYL_MEDIA_CODEC_INFO_H_

#include "../../foundation/media/include/ABase.h"
#include "../../foundation/media/include/AString.h"

#include "../../foundation/system/core/types.h"
#include "../../foundation/system/core/utils/Errors.h"
#include "../../foundation/system/core/utils/KeyedVector.h"
#include "../../foundation/system/core/utils/RefBase.h"
#include "../../foundation/system/core/utils/Vector.h"
#include "../../foundation/system/core/utils/StrongPointer.h"

LIYL_NAMESPACE_START

struct AMessage;

typedef KeyedVector<AString, AString> CodecSettings;

class MediaCodecInfo : public RefBase {
public:
    struct ProfileLevel {
        uint32_t mProfile;
        uint32_t mLevel;
    };

    struct Capabilities : public RefBase {
        enum {
            // decoder flags
            kFlagSupportsAdaptivePlayback = 1 << 0,
            kFlagSupportsSecurePlayback = 1 << 1,
            kFlagSupportsTunneledPlayback = 1 << 2,

            // encoder flags
            kFlagSupportsIntraRefresh = 1 << 0,

        };

        void getSupportedProfileLevels(Vector<ProfileLevel> *profileLevels) const;
        void getSupportedColorFormats(Vector<uint32_t> *colorFormats) const;
        uint32_t getFlags() const;
        const sp<AMessage> getDetails() const;

    protected:
        Vector<ProfileLevel> mProfileLevels;
        Vector<uint32_t> mColorFormats;
        uint32_t mFlags;
        sp<AMessage> mDetails;

        Capabilities();

    private:

        DISALLOW_EVIL_CONSTRUCTORS(Capabilities);

        friend class MediaCodecInfo;
    };

    // Use a subclass to allow setting fields on construction without allowing
    // to do the same throughout the framework.
    struct CapabilitiesBuilder : public Capabilities {
        void addProfileLevel(uint32_t profile, uint32_t level);
        void addColorFormat(uint32_t format);
        void addFlags(uint32_t flags);
    };

    bool isEncoder() const;
    bool hasQuirk(const char *name) const;
    void getSupportedMimes(Vector<AString> *mimes) const;
    const sp<Capabilities> getCapabilitiesFor(const char *mime) const;
    const char *getCodecName() const;


private:
    // variable set only in constructor - these are accessed by MediaCodecList
    // to avoid duplication of same variables
    AString mName;
    bool mIsEncoder;
    bool mHasSoleMime; // was initialized with mime

    Vector<AString> mQuirks;
    KeyedVector<AString, sp<Capabilities> > mCaps;

    sp<Capabilities> mCurrentCaps; // currently initalized capabilities

    ssize_t getCapabilityIndex(const char *mime) const;

    /* Methods used by MediaCodecList to construct the info
     * object from XML.
     *
     * After info object is created:
     * - additional quirks can be added
     * - additional mimes can be added
     *   - OMX codec capabilities can be set for the current mime-type
     *   - a capability detail can be set for the current mime-type
     *   - a feature can be set for the current mime-type
     *   - info object can be completed when parsing of a mime-type is done
     */
    MediaCodecInfo(AString name, bool encoder, const char *mime);
    void addQuirk(const char *name);
    status_t addMime(const char *mime);
    status_t updateMime(const char *mime);

    status_t initializeCapabilities(const sp<Capabilities> &caps);
    void addDetail(const AString &key, const AString &value);
    void addFeature(const AString &key, int32_t value);
    void addFeature(const AString &key, const char *value);
    void removeMime(const char *mime);
    void complete();

    DISALLOW_EVIL_CONSTRUCTORS(MediaCodecInfo);

    friend class MediaCodecList;
};

LIYL_NAMESPACE_END

#endif  // LIYL_MEDIA_CODEC_INFO_H_


