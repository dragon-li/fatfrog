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

#ifndef LIYL_HARDWARE_API_H_

#define LIYL_HARDWARE_API_H_


#include "VideoAPI.h"
#include "../../prebuild/openmax/OMX_Component.h"

namespace liyl {


// A pointer to this struct is passed to OMX_SetParameter() when the extension
// index "OMX.google.android.index.prepareForAdaptivePlayback" is given.
//
// This method is used to signal a video decoder, that the user has requested
// seamless resolution change support (if bEnable is set to OMX_TRUE).
// nMaxFrameWidth and nMaxFrameHeight are the dimensions of the largest
// anticipated frames in the video.  If bEnable is OMX_FALSE, no resolution
// change is expected, and the nMaxFrameWidth/Height fields are unused.
//
// If the decoder supports dynamic output buffers, it may ignore this
// request.  Otherwise, it shall request resources in such a way so that it
// avoids full port-reconfiguration (due to output port-definition change)
// during resolution changes.
//
// DO NOT USE THIS STRUCTURE AS IT WILL BE REMOVED.  INSTEAD, IMPLEMENT
// METADATA SUPPORT FOR VIDEO DECODERS.
struct PrepareForAdaptivePlaybackParams {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_U32 nMaxFrameWidth;
    OMX_U32 nMaxFrameHeight;
};



// An enum OMX_COLOR_FormatAndroidOpaque to indicate an opaque colorformat
// is declared in media/stagefright/openmax/OMX_IVCommon.h
// This will inform the encoder that the actual
// colorformat will be relayed by the GRalloc Buffers.
// OMX_COLOR_FormatAndroidOpaque  = 0x7F000001,

// A pointer to this struct is passed to OMX_SetParameter when the extension
// index for the 'OMX.google.android.index.prependSPSPPSToIDRFrames' extension
// is given.
// A successful result indicates that future IDR frames will be prefixed by
// SPS/PPS.
struct PrependSPSPPSToIDRFramesParams {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bEnable;
};


// Color space description (aspects) parameters.
// This is passed via OMX_SetConfig or OMX_GetConfig to video encoders and decoders when the
// 'OMX.google.android.index.describeColorAspects' extension is given. Component SHALL behave
// as described below if it supports this extension.
//
// bDataSpaceChanged and bRequestingDataSpace is assumed to be OMX_FALSE unless noted otherwise.
//
// VIDEO ENCODERS: the framework uses OMX_SetConfig to specify color aspects of the coded video.
// This may happen:
//   a) before the component transitions to idle state
//   b) before the input frame is sent via OMX_EmptyThisBuffer in executing state
//   c) during execution, just before an input frame with a different color aspect information
//      is sent.
//
// The framework also uses OMX_GetConfig to
//   d) verify the color aspects that will be written to the stream
//   e) (optional) verify the color aspects that should be reported to the container for a
//      given dataspace/pixelformat received
//
// 1. Encoders SHOULD maintain an internal color aspect state, initialized to Unspecified values.
//    This represents the values that will be written into the bitstream.
// 2. Upon OMX_SetConfig, they SHOULD update their internal state to the aspects received
//    (including Unspecified values). For specific aspect values that are not supported by the
//    codec standard, encoders SHOULD substitute Unspecified values; or they MAY use a suitable
//    alternative (e.g. to suggest the use of BT.709 EOTF instead of SMPTE 240M.)
// 3. OMX_GetConfig SHALL return the internal state (values that will be written).
// 4. OMX_SetConfig SHALL always succeed before receiving the first frame. It MAY fail afterwards,
//    but only if the configured values would change AND the component does not support updating the
//    color information to those values mid-stream. If component supports updating a portion of
//    the color information, those values should be updated in the internal state, and OMX_SetConfig
//    SHALL succeed. Otherwise, the internal state SHALL remain intact and OMX_SetConfig SHALL fail
//    with OMX_ErrorUnsupportedSettings.
// 5. When the framework receives an input frame with an unexpected dataspace, it will query
//    encoders for the color aspects that should be reported to the container using OMX_GetConfig
//    with bDataSpaceChanged set to OMX_TRUE, and nPixelFormat/nDataSpace containing the new
//    format/dataspace values. This allows vendors to use extended dataspace during capture and
//    composition (e.g. screenrecord) - while performing color-space conversion inside the encoder -
//    and encode and report a different color-space information in the bitstream/container.
//    sColorAspects contains the requested color aspects by the client for reference, which may
//    include aspects not supported by the encoding. This is used together with guidance for
//    dataspace selection; see 6. below.
//
// VIDEO DECODERS: the framework uses OMX_SetConfig to specify the default color aspects to use
// for the video.
// This may happen:
//   a) before the component transitions to idle state
//   b) during execution, when the resolution or the default color aspects change.
//
// The framework also uses OMX_GetConfig to
//   c) get the final color aspects reported by the coded bitstream after taking the default values
//      into account.
//
// 1. Decoders should maintain two color aspect states - the default state as reported by the
//    framework, and the coded state as reported by the bitstream - as each state can change
//    independently from the other.
// 2. Upon OMX_SetConfig, it SHALL update its default state regardless of whether such aspects
//    could be supplied by the component bitstream. (E.g. it should blindly support all enumeration
//    values, even unknown ones, and the Other value). This SHALL always succeed.
// 3. Upon OMX_GetConfig, the component SHALL return the final color aspects by replacing
//    Unspecified coded values with the default values. This SHALL always succeed.
// 4. Whenever the component processes color aspect information in the bitstream even with an
//    Unspecified value, it SHOULD update its internal coded state with that information just before
//    the frame with the new information would be outputted, and the component SHALL signal an
//    OMX_EventPortSettingsChanged event with data2 set to the extension index.
// NOTE: Component SHOULD NOT signal a separate event purely for color aspect change, if it occurs
//    together with a port definition (e.g. size) or crop change.
// 5. If the aspects a component encounters in the bitstream cannot be represented with enumeration
//    values as defined below, the component SHALL set those aspects to Other. Restricted values in
//    the bitstream SHALL be treated as defined by the relevant bitstream specifications/standards,
//    or as Unspecified, if not defined.
//
// BOTH DECODERS AND ENCODERS: the framework uses OMX_GetConfig during idle and executing state to
//   f) (optional) get guidance for the dataspace to set for given color aspects, by setting
//      bRequestingDataSpace to OMX_TRUE. The component SHALL return OMX_ErrorUnsupportedSettings
//      IF it does not support this request.
//
// 6. This is an information request that can happen at any time, independent of the normal
//    configuration process. This allows vendors to use extended dataspace during capture, playback
//    and composition - while performing color-space conversion inside the component. Component
//    SHALL set the desired dataspace into nDataSpace. Otherwise, it SHALL return
//    OMX_ErrorUnsupportedSettings to let the framework choose a nearby standard dataspace.
//
// 6.a. For encoders, this query happens before the first frame is received using surface encoding.
//    This allows the encoder to use a specific dataspace for the color aspects (e.g. because the
//    device supports additional dataspaces, or because it wants to perform color-space extension
//    to facilitate a more optimal rendering/capture pipeline.).
//
// 6.b. For decoders, this query happens before the first frame, and every time the color aspects
//    change, while using surface buffers. This allows the decoder to use a specific dataspace for
//    the color aspects (e.g. because the device supports additional dataspaces, or because it wants
//    to perform color-space extension by inline color-space conversion to facilitate a more optimal
//    rendering pipeline.).
//
// Note: the size of sAspects may increase in the future by additional fields.
// Implementations SHOULD NOT require a certain size.
struct DescribeColorAspectsParams {
    OMX_U32 nSize;                 // IN
    OMX_VERSIONTYPE nVersion;      // IN
    OMX_U32 nPortIndex;            // IN
    OMX_BOOL bRequestingDataSpace; // IN
    OMX_BOOL bDataSpaceChanged;    // IN
    OMX_U32 nPixelFormat;          // IN
    OMX_U32 nDataSpace;            // OUT
    ColorAspects sAspects;         // IN/OUT
};




struct DescribeColorFormat2Params {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    // input: parameters from OMX_VIDEO_PORTDEFINITIONTYPE
    OMX_COLOR_FORMATTYPE eColorFormat;
    OMX_U32 nFrameWidth;
    OMX_U32 nFrameHeight;
    OMX_U32 nStride;
    OMX_U32 nSliceHeight;
    OMX_BOOL bUsingNativeBuffers;

    // output: fill out the MediaImage2 fields
    MediaImage2 sMediaImage;

};

}; //namespace liyl

#endif  // HARDWARE_API_H_
