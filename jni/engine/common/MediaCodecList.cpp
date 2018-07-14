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

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaCodecList"

#include "../../foundation/media/include/ADebug.h"
#include "../../foundation/system/core/threads.h"
#include "../api/IMediaCodecList.h"
#include "MediaCodecInfo.h"
#include "MediaCodecList.h"
#include "../../common/include/MediaErrors.h"




LIYL_NAMESPACE_START


static Mutex sInitMutex;

static bool parseBoolean(const char *s) {
    if (!strcasecmp(s, "true") || !strcasecmp(s, "yes") || !strcasecmp(s, "y")) {
        return true;
    }
    char *end;
    unsigned long res = strtoul(s, &end, 10);
    return *s != '\0' && *end == '\0' && res > 0;
}

static bool isProfilingNeeded() {
        return false;
}

// static
sp<IMediaCodecList> MediaCodecList::sCodecList;

// static
void *MediaCodecList::profilerThreadWrapper(void * /*arg*/) {
    LLOGE("Don't support profilerThreadWrapper.");
    return NULL;
}

// static
sp<IMediaCodecList> MediaCodecList::getLocalInstance() {
    Mutex::Autolock autoLock(sInitMutex);

    if (sCodecList == NULL) {
        MediaCodecList *codecList = new MediaCodecList;
        if (codecList->initCheck() == OK) {
            sCodecList = codecList;

            if (isProfilingNeeded()) {
                LLOGV("Codec profiling needed, will be run in separated thread.");
                pthread_t profiler;
                if (pthread_create(&profiler, NULL, profilerThreadWrapper, NULL) != 0) {
                    LLOGW("Failed to create thread for codec profiling.");
                }
            }
        } else {
            // failure to initialize may be temporary. retry on next call.
            delete codecList;
        }
    }

    return sCodecList;
}

MediaCodecList::MediaCodecList()
    : mInitCheck(NO_INIT),
      mUpdate(false),
      mGlobalSettings(new AMessage()) {
    parseTopLevelXMLFile("/etc/media_codecs.xml");
}

void MediaCodecList::parseTopLevelXMLFile(const char *codecs_xml, bool ignore_errors) {

    mInitCheck = OK; // keeping this here for safety
    mCurrentSection = SECTION_TOPLEVEL;
    mDepth = 0;

    //TODO
    /* parse xml start*/
    startElementHandler("Decoders",NULL);
    //<MediaCodec name="OMX.google.mpeg4.decoder" type="video/mp4v-es" />
    //<MediaCodec name="OMX.google.h264.decoder" type="video/avc" />
    static const char* MediaCodecXml[] =  {
          "name",
          "OMX.liyl.aac.decoder",
          "type",
          "audio/aac",
          "name",
          "OMX.liyl.h264.decoder",
          "type",
          "video/avc",
          NULL
    };
    startElementHandler("MediaCodec",MediaCodecXml);
    

    endElementHandler("MediaCodec");
    endElementHandler("Decoders");

    /* parse xml end*/
    for (size_t i = mCodecInfos.size(); i > 0;) {
        i--;
        const MediaCodecInfo &info = *mCodecInfos.itemAt(i).get();
        if (info.mCaps.size() == 0) {
            // No types supported by this component???
            LLOGW("Component %s does not support any type of media?",
                  info.mName.c_str());

            mCodecInfos.removeAt(i);
        } else {
            for (size_t type_ix = 0; type_ix < info.mCaps.size(); ++type_ix) {
                AString mime = info.mCaps.keyAt(type_ix);
                const sp<MediaCodecInfo::Capabilities> &caps = info.mCaps.valueAt(type_ix);

                LLOGV("%s codec info for %s: %s", info.mName.c_str(), mime.c_str(),
                        caps->getDetails()->debugString().c_str());
                LLOGV("    flags=%d", caps->getFlags());
                {
                    Vector<uint32_t> colorFormats;
                    caps->getSupportedColorFormats(&colorFormats);
                    AString nice;
                    for (size_t ix = 0; ix < colorFormats.size(); ix++) {
                        if (ix > 0) {
                            nice.append(", ");
                        }
                        nice.append(colorFormats.itemAt(ix));
                    }
                    LLOGV("    colors=[%s]", nice.c_str());
                }
                {
                    Vector<MediaCodecInfo::ProfileLevel> profileLevels;
                    caps->getSupportedProfileLevels(&profileLevels);
                    AString nice;
                    for (size_t ix = 0; ix < profileLevels.size(); ix++) {
                        if (ix > 0) {
                            nice.append(", ");
                        }
                        const MediaCodecInfo::ProfileLevel &pl =
                            profileLevels.itemAt(ix);
                        nice.append(pl.mProfile);
                        nice.append("/");
                        nice.append(pl.mLevel);
                    }
                    LLOGV("    levels=[%s]", nice.c_str());
                }
                {
                    AString quirks;
                    for (size_t ix = 0; ix < info.mQuirks.size(); ix++) {
                        if (ix > 0) {
                            quirks.append(", ");
                        }
                        quirks.append(info.mQuirks[ix]);
                    }
                    LLOGV("    quirks=[%s]", quirks.c_str());
                }
            }
        }
    }
#if 0
    for (size_t i = 0; i < mCodecInfos.size(); ++i) {
        const CodecInfo &info = mCodecInfos.itemAt(i);

        AString line = info.mName;
        line.append(" supports ");
        for (size_t j = 0; j < mTypes.size(); ++j) {
            uint32_t value = mTypes.valueAt(j);

            if (info.mTypes & (1ul << value)) {
                line.append(mTypes.keyAt(j));
                line.append(" ");
            }
        }

        LLOGI("%s", line.c_str());
    }
#endif
}

MediaCodecList::~MediaCodecList() {
}

status_t MediaCodecList::initCheck() const {
    return mInitCheck;
}



void MediaCodecList::startElementHandler(
        const char *name, const char **attrs) {
    if (mInitCheck != OK) {
        return;
    }

    bool inType = true;

    switch (mCurrentSection) {
        case SECTION_TOPLEVEL:
        {
            if (!strcmp(name, "Decoders")) {
                mCurrentSection = SECTION_DECODERS;
            } else if (!strcmp(name, "Encoders")) {
                mCurrentSection = SECTION_ENCODERS;
            } else if (!strcmp(name, "Settings")) {
                mCurrentSection = SECTION_SETTINGS;
            }
            break;
        }

        case SECTION_SETTINGS:
        {
            if (!strcmp(name, "Setting")) {
                mInitCheck = addSettingFromAttributes(attrs);
            }
            break;
        }

        case SECTION_DECODERS:
        {
            if (!strcmp(name, "MediaCodec")) {
                mInitCheck =
                    addMediaCodecFromAttributes(false /* encoder */, attrs);

                mCurrentSection = SECTION_DECODER;
            }
            break;
        }

        case SECTION_ENCODERS:
        {
            if (!strcmp(name, "MediaCodec")) {
                mInitCheck =
                    addMediaCodecFromAttributes(true /* encoder */, attrs);

                mCurrentSection = SECTION_ENCODER;
            }
            break;
        }

        case SECTION_DECODER:
        case SECTION_ENCODER:
        {
            if (!strcmp(name, "Quirk")) {
                mInitCheck = addQuirk(attrs);
            } else if (!strcmp(name, "Type")) {
                mInitCheck = addTypeFromAttributes(attrs);
                mCurrentSection =
                    (mCurrentSection == SECTION_DECODER
                            ? SECTION_DECODER_TYPE : SECTION_ENCODER_TYPE);
            }
        }
        inType = false;
        // fall through

        case SECTION_DECODER_TYPE:
        case SECTION_ENCODER_TYPE:
        {
            // ignore limits and features specified outside of type
            bool outside = !inType && !mCurrentInfo->mHasSoleMime;
            if (outside && (!strcmp(name, "Limit") || !strcmp(name, "Feature"))) {
                LLOGW("ignoring %s specified outside of a Type", name);
            } else if (!strcmp(name, "Limit")) {
                mInitCheck = addLimit(attrs);
            } else if (!strcmp(name, "Feature")) {
                mInitCheck = addFeature(attrs);
            }
            break;
        }

        default:
            break;
    }

    ++mDepth;
}

void MediaCodecList::endElementHandler(const char *name) {
    if (mInitCheck != OK) {
        return;
    }

    switch (mCurrentSection) {
        case SECTION_SETTINGS:
        {
            if (!strcmp(name, "Settings")) {
                mCurrentSection = SECTION_TOPLEVEL;
            }
            break;
        }

        case SECTION_DECODERS:
        {
            if (!strcmp(name, "Decoders")) {
                mCurrentSection = SECTION_TOPLEVEL;
            }
            break;
        }

        case SECTION_ENCODERS:
        {
            if (!strcmp(name, "Encoders")) {
                mCurrentSection = SECTION_TOPLEVEL;
            }
            break;
        }

        case SECTION_DECODER_TYPE:
        case SECTION_ENCODER_TYPE:
        {
            if (!strcmp(name, "Type")) {
                mCurrentSection =
                    (mCurrentSection == SECTION_DECODER_TYPE
                            ? SECTION_DECODER : SECTION_ENCODER);

                mCurrentInfo->complete();
            }
            break;
        }

        case SECTION_DECODER:
        {
            if (!strcmp(name, "MediaCodec")) {
                mCurrentSection = SECTION_DECODERS;
                mCurrentInfo->complete();
                mCurrentInfo = NULL;
            }
            break;
        }

        case SECTION_ENCODER:
        {
            if (!strcmp(name, "MediaCodec")) {
                mCurrentSection = SECTION_ENCODERS;
                mCurrentInfo->complete();;
                mCurrentInfo = NULL;
            }
            break;
        }

        default:
            break;
    }

    --mDepth;
}

status_t MediaCodecList::addSettingFromAttributes(const char **attrs) {
    const char *name = NULL;
    const char *value = NULL;
    const char *update = NULL;

    size_t i = 0;
    while (attrs[i] != NULL) {
        if (!strcmp(attrs[i], "name")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            name = attrs[i + 1];
            ++i;
        } else if (!strcmp(attrs[i], "value")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            value = attrs[i + 1];
            ++i;
        } else if (!strcmp(attrs[i], "update")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            update = attrs[i + 1];
            ++i;
        } else {
            return -EINVAL;
        }

        ++i;
    }

    if (name == NULL || value == NULL) {
        return -EINVAL;
    }

    mUpdate = (update != NULL) && parseBoolean(update);
    if (mUpdate != mGlobalSettings->contains(name)) {
        return -EINVAL;
    }

    mGlobalSettings->setString(name, value);
    return OK;
}

void MediaCodecList::setCurrentCodecInfo(bool encoder, const char *name, const char *type) {
    for (size_t i = 0; i < mCodecInfos.size(); ++i) {
        if (AString(name) == mCodecInfos[i]->getCodecName()) {
            if (mCodecInfos[i]->getCapabilitiesFor(type) == NULL) {
                LLOGW("Overrides with an unexpected mime %s", type);
                // Create a new MediaCodecInfo (but don't add it to mCodecInfos) to hold the
                // overrides we don't want.
                mCurrentInfo = new MediaCodecInfo(name, encoder, type);
            } else {
                mCurrentInfo = mCodecInfos.editItemAt(i);
                mCurrentInfo->updateMime(type);  // to set the current cap
            }
            return;
        }
    }
    mCurrentInfo = new MediaCodecInfo(name, encoder, type);
    // The next step involves trying to load the codec, which may
    // fail.  Only list the codec if this succeeds.
    // However, keep mCurrentInfo object around until parsing
    // of full codec info is completed.
    if (initializeCapabilities(type) == OK) {
        mCodecInfos.push_back(mCurrentInfo);
    }
}

status_t MediaCodecList::addMediaCodecFromAttributes(
        bool encoder, const char **attrs) {
    const char *name = NULL;
    const char *type = NULL;
    const char *update = NULL;

    size_t i = 0;
    while (attrs[i] != NULL) {
        if (!strcmp(attrs[i], "name")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            name = attrs[i + 1];
            ++i;
        } else if (!strcmp(attrs[i], "type")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            type = attrs[i + 1];
            ++i;
        } else if (!strcmp(attrs[i], "update")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            update = attrs[i + 1];
            ++i;
        } else {
            return -EINVAL;
        }

        ++i;
    }

    if (name == NULL) {
        return -EINVAL;
    }

    mUpdate = (update != NULL) && parseBoolean(update);
    ssize_t index = -1;
    for (size_t i = 0; i < mCodecInfos.size(); ++i) {
        if (AString(name) == mCodecInfos[i]->getCodecName()) {
            index = i;
        }
    }
    if (mUpdate != (index >= 0)) {
        return -EINVAL;
    }

    if (index >= 0) {
        // existing codec
        mCurrentInfo = mCodecInfos.editItemAt(index);
        if (type != NULL) {
            // existing type
            if (mCodecInfos[index]->getCapabilitiesFor(type) == NULL) {
                return -EINVAL;
            }
            mCurrentInfo->updateMime(type);
        }
    } else {
        // new codec
        mCurrentInfo = new MediaCodecInfo(name, encoder, type);
        // The next step involves trying to load the codec, which may
        // fail.  Only list the codec if this succeeds.
        // However, keep mCurrentInfo object around until parsing
        // of full codec info is completed.
        if (initializeCapabilities(type) == OK) {
            mCodecInfos.push_back(mCurrentInfo);
        }
    }

    return OK;
}

status_t MediaCodecList::QuerryCapabilities( 
        const AString &name, const AString &mime,
        sp<MediaCodecInfo::Capabilities> *caps) {
    (*caps).clear();

    sp<MediaCodecInfo::CapabilitiesBuilder> builder = new MediaCodecInfo::CapabilitiesBuilder();
    bool isVideo = mime.startsWithIgnoreCase("video/");

    if (isVideo) {//AVC
        static const CodecProfileLevel avcProfileLevels[] = {
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1  },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2  },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3  },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4  },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5  },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
            { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel52 },

            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel1  },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel1b },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel11 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel12 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel13 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel2  },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel21 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel22 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel3  },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel31 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel32 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel4  },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel41 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel42 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel5  },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel51 },
            { OMX_VIDEO_AVCProfileMain,     OMX_VIDEO_AVCLevel52 },

            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel1  },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel1b },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel11 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel12 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel13 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel2  },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel21 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel22 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel3  },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel31 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel32 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel4  },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel41 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel42 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel5  },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel51 },
            { OMX_VIDEO_AVCProfileHigh,     OMX_VIDEO_AVCLevel52 },
        };
        for (OMX_U32 index = 0; index < ARRAY_SIZE(avcProfileLevels) ; ++index) {
            builder->addProfileLevel(avcProfileLevels[index].mProfile, avcProfileLevels[index].mLevel);
        }
        //Color format query
        builder->addColorFormat(OMX_COLOR_FormatYUV420Planar);
        builder->addFlags(MediaCodecInfo::Capabilities::kFlagSupportsAdaptivePlayback);

    } else {//AAC
        static const CodecProfileLevel aacSupportedProfiles[] = {
            {OMX_AUDIO_AACObjectLC,    0},
            {OMX_AUDIO_AACObjectHE,    0},
            {OMX_AUDIO_AACObjectHE_PS, 0},
            {OMX_AUDIO_AACObjectLD,    0},
            {OMX_AUDIO_AACObjectELD,   0},
        };
        // For audio, level is ignored.
        for (OMX_U32 index = 0; index < ARRAY_SIZE(aacSupportedProfiles) ; ++index) {
            builder->addProfileLevel(aacSupportedProfiles[index].mProfile, 0 /* level */);
        }

    }

    *caps = builder;
    return OK;

}

status_t MediaCodecList::initializeCapabilities(const char *type) {
    if (type == NULL) {
        return OK;
    }

    LLOGV("initializeCapabilities %s:%s",
            mCurrentInfo->mName.c_str(), type);

    sp<MediaCodecInfo::Capabilities> caps;
    status_t err = QuerryCapabilities(
            mCurrentInfo->mName.c_str(),
            type,
            &caps);
    if (err != OK) {
        return err;
    }

    return mCurrentInfo->initializeCapabilities(caps);
}

status_t MediaCodecList::addQuirk(const char **attrs) {
    const char *name = NULL;

    size_t i = 0;
    while (attrs[i] != NULL) {
        if (!strcmp(attrs[i], "name")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            name = attrs[i + 1];
            ++i;
        } else {
            return -EINVAL;
        }

        ++i;
    }

    if (name == NULL) {
        return -EINVAL;
    }

    mCurrentInfo->addQuirk(name);
    return OK;
}

status_t MediaCodecList::addTypeFromAttributes(const char **attrs) {
    const char *name = NULL;
    const char *update = NULL;

    size_t i = 0;
    while (attrs[i] != NULL) {
        if (!strcmp(attrs[i], "name")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            name = attrs[i + 1];
            ++i;
        } else if (!strcmp(attrs[i], "update")) {
            if (attrs[i + 1] == NULL) {
                return -EINVAL;
            }
            update = attrs[i + 1];
            ++i;
        } else {
            return -EINVAL;
        }

        ++i;
    }

    if (name == NULL) {
        return -EINVAL;
    }

    bool isExistingType = (mCurrentInfo->getCapabilitiesFor(name) != NULL);
    if (mUpdate != isExistingType) {
        return -EINVAL;
    }

    status_t ret;
    if (mUpdate) {
        ret = mCurrentInfo->updateMime(name);
    } else {
        ret = mCurrentInfo->addMime(name);
    }

    if (ret != OK) {
        return ret;
    }

    // The next step involves trying to load the codec, which may
    // fail.  Handle this gracefully (by not reporting such mime).
    if (!mUpdate && initializeCapabilities(name) != OK) {
        mCurrentInfo->removeMime(name);
    }
    return OK;
}

// legacy method for non-advanced codecs
ssize_t MediaCodecList::findCodecByType(
        const char *type, bool encoder, size_t startIndex) const {

    size_t numCodecs = mCodecInfos.size();
    for (; startIndex < numCodecs; ++startIndex) {
        const MediaCodecInfo &info = *mCodecInfos.itemAt(startIndex).get();

        if (info.isEncoder() != encoder) {
            continue;
        }
        sp<MediaCodecInfo::Capabilities> capabilities = info.getCapabilitiesFor(type);
        if (capabilities == NULL) {
            continue;
        }

        return startIndex;
    }

    return -ENOENT;
}

static status_t limitFoundMissingAttr(AString name, const char *attr, bool found = true) {
    LLOGE("limit '%s' with %s'%s' attribute", name.c_str(),
            (found ? "" : "no "), attr);
    return -EINVAL;
}

static status_t limitError(AString name, const char *msg) {
    LLOGE("limit '%s' %s", name.c_str(), msg);
    return -EINVAL;
}

static status_t limitInvalidAttr(AString name, const char *attr, AString value) {
    LLOGE("limit '%s' with invalid '%s' attribute (%s)", name.c_str(),
            attr, value.c_str());
    return -EINVAL;
}

status_t MediaCodecList::addLimit(const char **attrs) {
    sp<AMessage> msg = new AMessage();

    size_t i = 0;
    while (attrs[i] != NULL) {
        if (attrs[i + 1] == NULL) {
            return -EINVAL;
        }

        // attributes with values
        if (!strcmp(attrs[i], "name")
                || !strcmp(attrs[i], "default")
                || !strcmp(attrs[i], "in")
                || !strcmp(attrs[i], "max")
                || !strcmp(attrs[i], "min")
                || !strcmp(attrs[i], "range")
                || !strcmp(attrs[i], "ranges")
                || !strcmp(attrs[i], "scale")
                || !strcmp(attrs[i], "value")) {
            msg->setString(attrs[i], attrs[i + 1]);
            ++i;
        } else {
            return -EINVAL;
        }
        ++i;
    }

    AString name;
    if (!msg->findString("name", &name)) {
        LLOGE("limit with no 'name' attribute");
        return -EINVAL;
    }

    // size, blocks, bitrate, frame-rate, blocks-per-second, aspect-ratio,
    // measured-frame-rate, measured-blocks-per-second: range
    // quality: range + default + [scale]
    // complexity: range + default
    bool found;

    if (name == "aspect-ratio" || name == "bitrate" || name == "block-count"
            || name == "blocks-per-second" || name == "complexity"
            || name == "frame-rate" || name == "quality" || name == "size"
            || name == "measured-blocks-per-second" || name.startsWith("measured-frame-rate-")) {
        AString min, max;
        if (msg->findString("min", &min) && msg->findString("max", &max)) {
            min.append("-");
            min.append(max);
            if (msg->contains("range") || msg->contains("value")) {
                return limitError(name, "has 'min' and 'max' as well as 'range' or "
                        "'value' attributes");
            }
            msg->setString("range", min);
        } else if (msg->contains("min") || msg->contains("max")) {
            return limitError(name, "has only 'min' or 'max' attribute");
        } else if (msg->findString("value", &max)) {
            min = max;
            min.append("-");
            min.append(max);
            if (msg->contains("range")) {
                return limitError(name, "has both 'range' and 'value' attributes");
            }
            msg->setString("range", min);
        }

        AString range, scale = "linear", def, in_;
        if (!msg->findString("range", &range)) {
            return limitError(name, "with no 'range', 'value' or 'min'/'max' attributes");
        }

        if ((name == "quality" || name == "complexity") ^
                (found = msg->findString("default", &def))) {
            return limitFoundMissingAttr(name, "default", found);
        }
        if (name != "quality" && msg->findString("scale", &scale)) {
            return limitFoundMissingAttr(name, "scale");
        }
        if ((name == "aspect-ratio") ^ (found = msg->findString("in", &in_))) {
            return limitFoundMissingAttr(name, "in", found);
        }

        if (name == "aspect-ratio") {
            if (!(in_ == "pixels") && !(in_ == "blocks")) {
                return limitInvalidAttr(name, "in", in_);
            }
            in_.erase(5, 1); // (pixel|block)-aspect-ratio
            in_.append("-");
            in_.append(name);
            name = in_;
        }
        if (name == "quality") {
            mCurrentInfo->addDetail("quality-scale", scale);
        }
        if (name == "quality" || name == "complexity") {
            AString tag = name;
            tag.append("-default");
            mCurrentInfo->addDetail(tag, def);
        }
        AString tag = name;
        tag.append("-range");
        mCurrentInfo->addDetail(tag, range);
    } else {
        AString max, value, ranges;
        if (msg->contains("default")) {
            return limitFoundMissingAttr(name, "default");
        } else if (msg->contains("in")) {
            return limitFoundMissingAttr(name, "in");
        } else if ((name == "channel-count" || name == "concurrent-instances") ^
                (found = msg->findString("max", &max))) {
            return limitFoundMissingAttr(name, "max", found);
        } else if (msg->contains("min")) {
            return limitFoundMissingAttr(name, "min");
        } else if (msg->contains("range")) {
            return limitFoundMissingAttr(name, "range");
        } else if ((name == "sample-rate") ^
                (found = msg->findString("ranges", &ranges))) {
            return limitFoundMissingAttr(name, "ranges", found);
        } else if (msg->contains("scale")) {
            return limitFoundMissingAttr(name, "scale");
        } else if ((name == "alignment" || name == "block-size") ^
                (found = msg->findString("value", &value))) {
            return limitFoundMissingAttr(name, "value", found);
        }

        if (max.size()) {
            AString tag = "max-";
            tag.append(name);
            mCurrentInfo->addDetail(tag, max);
        } else if (value.size()) {
            mCurrentInfo->addDetail(name, value);
        } else if (ranges.size()) {
            AString tag = name;
            tag.append("-ranges");
            mCurrentInfo->addDetail(tag, ranges);
        } else {
            LLOGW("Ignoring unrecognized limit '%s'", name.c_str());
        }
    }
    return OK;
}

status_t MediaCodecList::addFeature(const char **attrs) {
    size_t i = 0;
    const char *name = NULL;
    int32_t optional = -1;
    int32_t required = -1;
    const char *value = NULL;

    while (attrs[i] != NULL) {
        if (attrs[i + 1] == NULL) {
            return -EINVAL;
        }

        // attributes with values
        if (!strcmp(attrs[i], "name")) {
            name = attrs[i + 1];
            ++i;
        } else if (!strcmp(attrs[i], "optional") || !strcmp(attrs[i], "required")) {
            int value = (int)parseBoolean(attrs[i + 1]);
            if (!strcmp(attrs[i], "optional")) {
                optional = value;
            } else {
                required = value;
            }
            ++i;
        } else if (!strcmp(attrs[i], "value")) {
            value = attrs[i + 1];
            ++i;
        } else {
            return -EINVAL;
        }
        ++i;
    }
    if (name == NULL) {
        LLOGE("feature with no 'name' attribute");
        return -EINVAL;
    }

    if (optional == required && optional != -1) {
        LLOGE("feature '%s' is both/neither optional and required", name);
        return -EINVAL;
    }

    if ((optional != -1 || required != -1) && (value != NULL)) {
        LLOGE("feature '%s' has both a value and optional/required attribute", name);
        return -EINVAL;
    }

    if (value != NULL) {
        mCurrentInfo->addFeature(name, value);
    } else {
        mCurrentInfo->addFeature(name, (required == 1) || (optional == 0));
    }
    return OK;
}

ssize_t MediaCodecList::findCodecByName(const char *name) const {
    for (size_t i = 0; i < mCodecInfos.size(); ++i) {
        const MediaCodecInfo &info = *mCodecInfos.itemAt(i).get();

        if (info.mName == name) {
            return i;
        }
    }

    return -ENOENT;
}

size_t MediaCodecList::countCodecs() const {
    return mCodecInfos.size();
}


const sp<AMessage> MediaCodecList::getGlobalSettings() const {
    return mGlobalSettings;
}

//static
void MediaCodecList::findMatchingCodecs(
		const char *mime, bool encoder, uint32_t flags, Vector<AString> *matches) {
	matches->clear();

	const sp<IMediaCodecList> list = getLocalInstance();
	if (list == NULL) {
		return;
	}

	size_t index = 0;
	for (;;) {
		ssize_t matchIndex = list->findCodecByType(mime, encoder, index);

		if (matchIndex < 0) {
			break;
		}

		index = matchIndex + 1;

		const sp<MediaCodecInfo> info = list->getCodecInfo(matchIndex);
		CHECK(info != NULL);
		AString componentName = info->getCodecName();

		matches->push(componentName);
		LLOGV("matching '%s'", componentName.c_str());
	}

}

// static
uint32_t MediaCodecList::getQuirksFor(const char *componentName) {
	const sp<IMediaCodecList> list = getLocalInstance();
	if (list == NULL) {
		return 0;
	}

	ssize_t ix = list->findCodecByName(componentName);
	if (ix < 0) {
		return 0;
	}

	const sp<MediaCodecInfo> info = list->getCodecInfo(ix);

	uint32_t quirks = 0;
	if (info->hasQuirk("requires-allocate-on-input-ports")) {
		//quirks |= ACodec::kRequiresAllocateBufferOnInputPorts;
	}
	if (info->hasQuirk("requires-allocate-on-output-ports")) {
		//quirks |= ACodec::kRequiresAllocateBufferOnOutputPorts;
	}

	return quirks;
}

LIYL_NAMESPACE_END

