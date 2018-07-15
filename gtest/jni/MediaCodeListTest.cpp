/*************************************************************************
  > File Name: HttpDataSourceTest.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2018年03月02日 星期五 11时08分56秒
 ************************************************************************/

//#include "../../jni/engine/api/IMediaCodecList.h"
#include "../../jni/engine/common/MediaCodecList.h"
#include "../../jni/common/include/MediaDefs.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string>
#include <errno.h>
using namespace std;
USING_NAMESPACE_LIYL;

class MediaCodecListTest : public ::testing::Test {
    public:
        virtual void SetUp(){
            //ENTER_FUNC;
        }
        virtual void TearDown(){
            //ESC_FUNC;
        }

    public:
};

TEST_F(MediaCodecListTest, test0) { // parallel  load
    const char *kMimeTypes[] = {
        MEDIA_MIMETYPE_VIDEO_AVC,
        MEDIA_MIMETYPE_AUDIO_AAC,
    };

    const char *codecType = "decoder";
    printf("%s profiles:\n", codecType);

    sp<IMediaCodecList> list = MediaCodecList::getLocalInstance();
    size_t numCodecs = list->countCodecs();
    printf("numCodecs %zu:\n", numCodecs);

    for (size_t k = 0; k < sizeof(kMimeTypes) / sizeof(kMimeTypes[0]); ++k) {
        printf("type '%s':\n", kMimeTypes[k]);

        for (size_t index = 0; index < numCodecs; ++index) {
            sp<MediaCodecInfo> info = list->getCodecInfo(index);
            if (info == NULL || info->isEncoder()) {
                continue;
            }
            sp<MediaCodecInfo::Capabilities> caps = info->getCapabilitiesFor(kMimeTypes[k]);
            if (caps == NULL) {
                continue;
            }
            printf("  %s '%s' supports ",
                    codecType, info->getCodecName());

            Vector<MediaCodecInfo::ProfileLevel> profileLevels;
            caps->getSupportedProfileLevels(&profileLevels);
            if (profileLevels.size() == 0) {
                printf("NOTHING.\n");
                continue;
            }

            for (size_t j = 0; j < profileLevels.size(); ++j) {
                const MediaCodecInfo::ProfileLevel &profileLevel = profileLevels[j];

                printf("%s%u/%u", j > 0 ? ", " : "",
                        profileLevel.mProfile, profileLevel.mLevel);
            }

            printf("\n");
        }
    }
    MediaCodecList::destoryLocalInstance();

}


