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

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaCodecInfo"
#include "../../foundation/system/core/utils/Log.h"

#include "../../omx/api/IOMX.h"

#include "MediaCodecInfo.h"

#include "../../foundation/media/include/ADebug.h"
#include "../../foundation/media/include/AMessage.h"

LIYL_NAMESPACE_START

void MediaCodecInfo::Capabilities::getSupportedProfileLevels(
        Vector<ProfileLevel> *profileLevels) const {
    profileLevels->clear();
    profileLevels->appendVector(mProfileLevels);
}

void MediaCodecInfo::Capabilities::getSupportedColorFormats(
        Vector<uint32_t> *colorFormats) const {
    colorFormats->clear();
    colorFormats->appendVector(mColorFormats);
}

uint32_t MediaCodecInfo::Capabilities::getFlags() const {
    return mFlags;
}

const sp<AMessage> MediaCodecInfo::Capabilities::getDetails() const {
    return mDetails;
}

MediaCodecInfo::Capabilities::Capabilities()
  : mFlags(0) {
    mDetails = new AMessage;
}


void MediaCodecInfo::CapabilitiesBuilder::addProfileLevel(uint32_t profile, uint32_t level) {
    ProfileLevel profileLevel;
    profileLevel.mProfile = profile;
    profileLevel.mLevel = level;
    mProfileLevels.push_back(profileLevel);
}

void MediaCodecInfo::CapabilitiesBuilder::addColorFormat(uint32_t format) {
    mColorFormats.push(format);
}

void MediaCodecInfo::CapabilitiesBuilder::addFlags(uint32_t flags) {
    mFlags |= flags;
}

bool MediaCodecInfo::isEncoder() const {
    return mIsEncoder;
}

bool MediaCodecInfo::hasQuirk(const char *name) const {
    for (size_t ix = 0; ix < mQuirks.size(); ix++) {
        if (mQuirks.itemAt(ix).equalsIgnoreCase(name)) {
            return true;
        }
    }
    return false;
}

void MediaCodecInfo::getSupportedMimes(Vector<AString> *mimes) const {
    mimes->clear();
    for (size_t ix = 0; ix < mCaps.size(); ix++) {
        mimes->push_back(mCaps.keyAt(ix));
    }
}

const sp<MediaCodecInfo::Capabilities>
MediaCodecInfo::getCapabilitiesFor(const char *mime) const {
    ssize_t ix = getCapabilityIndex(mime);
    if (ix >= 0) {
        return mCaps.valueAt(ix);
    }
    return NULL;
}

const char *MediaCodecInfo::getCodecName() const {
    return mName.c_str();
}


ssize_t MediaCodecInfo::getCapabilityIndex(const char *mime) const {
    for (size_t ix = 0; ix < mCaps.size(); ix++) {
        if (mCaps.keyAt(ix).equalsIgnoreCase(mime)) {
            return ix;
        }
    }
    return -1;
}

MediaCodecInfo::MediaCodecInfo(AString name, bool encoder, const char *mime)
    : mName(name),
      mIsEncoder(encoder),
      mHasSoleMime(false) {
    if (mime != NULL) {
        addMime(mime);
        mHasSoleMime = true;
    }
}

status_t MediaCodecInfo::addMime(const char *mime) {
    if (mHasSoleMime) {
        LLOGE("Codec '%s' already had its type specified", mName.c_str());
        return -EINVAL;
    }
    ssize_t ix = getCapabilityIndex(mime);
    if (ix >= 0) {
        mCurrentCaps = mCaps.valueAt(ix);
    } else {
        mCurrentCaps = new Capabilities();
        mCaps.add(AString(mime), mCurrentCaps);
    }
    return OK;
}

status_t MediaCodecInfo::updateMime(const char *mime) {
    ssize_t ix = getCapabilityIndex(mime);
    if (ix < 0) {
        LLOGE("updateMime mime not found %s", mime);
        return -EINVAL;
    }

    mCurrentCaps = mCaps.valueAt(ix);
    return OK;
}

void MediaCodecInfo::removeMime(const char *mime) {
    ssize_t ix = getCapabilityIndex(mime);
    if (ix >= 0) {
        mCaps.removeItemsAt(ix);
        // mCurrentCaps will be removed when completed
    }
}

status_t MediaCodecInfo::initializeCapabilities(const sp<Capabilities> &caps) {
    // TRICKY: copy data to mCurrentCaps as it is a reference to
    // an element of the capabilites map.
    mCurrentCaps->mColorFormats.clear();
    mCurrentCaps->mColorFormats.appendVector(caps->mColorFormats);
    mCurrentCaps->mProfileLevels.clear();
    mCurrentCaps->mProfileLevels.appendVector(caps->mProfileLevels);
    mCurrentCaps->mFlags = caps->mFlags;
    mCurrentCaps->mDetails = caps->mDetails;
    return OK;
}

void MediaCodecInfo::addQuirk(const char *name) {
    if (!hasQuirk(name)) {
        mQuirks.push(name);
    }
}

void MediaCodecInfo::complete() {
    mCurrentCaps = NULL;
}

void MediaCodecInfo::addDetail(const AString &key, const AString &value) {
    mCurrentCaps->mDetails->setString(key.c_str(), value.c_str());
}

void MediaCodecInfo::addFeature(const AString &key, int32_t value) {
    AString tag = "feature-";
    tag.append(key);
    mCurrentCaps->mDetails->setInt32(tag.c_str(), value);
}

void MediaCodecInfo::addFeature(const AString &key, const char *value) {
    AString tag = "feature-";
    tag.append(key);
    mCurrentCaps->mDetails->setString(tag.c_str(), value);
}

LIYL_NAMESPACE_END
