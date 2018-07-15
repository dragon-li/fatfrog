/*************************************************************************
	> File Name: HttpDownloaderTest.cpp
	> Author: liyunlong
	> Mail: liyunlong_88@126.com 
	> Created Time: 2018年03月02日 星期五 11时08分56秒
 ************************************************************************/

#include "../../jni/datasource/http/downloader/dl_utils.h"
#include "../../jni/datasource/http/downloader/ll_dummy_observer.h"
#include "../../jni/datasource/http/downloader/ll_loadmanager.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string>
#include <errno.h>
using namespace std;
USING_NAMESPACE_LIYL;

class HttpDownloaderTest: public ::testing::Test {
    public:
        virtual void SetUp(){
			ENTER_FUNC;
            mManager =  LL_LoadManager::getInstance().get();
            node_id node = -1;
        }
        virtual void TearDown(){
			ESC_FUNC;
        }

    public:
        LL_LoadManager* mManager;
};


TEST_F(HttpDownloaderTest, test9) { // startloading 


    const int NODE_NUM = 4;
    node_id nodes[NODE_NUM]= {0,};
    DataSpec* dataspec[NODE_NUM]= {0,};
    DummyObserver* observers[NODE_NUM]= {0,};
    char* tmp = (char*)malloc(512);
    sprintf(tmp,"%s","http://www.baidu.com");
	//wlan
    //sprintf(tmp,"%s","http://30.96.91.171/bbb_short.ffmpeg.1920x1080.mp4.libx264_10000kbps_30fps.libfaac_stereo_192kbps_48000Hz.mp4");
    char* userAgent = (char*)malloc(64);
    sprintf(userAgent,"%s","User-Agent: Android6.0");
    HTTPDownloadStatusInfo info;
    for(int i = 0 ;i < NODE_NUM; i++) {
        observers[i]= new DummyObserver(mManager); 
        dataspec[i] = new DataSpec();
        dataspec[i]->position = 0ll;
        dataspec[i]->connectTimeoutMs= 20*1000;
        //dataspec[i]->length = (i+1)*1000000ll;
        dataspec[i]->length = 4000000ll;
        dataspec[i]->url = tmp; 
        dataspec[i]->headUserAgent = userAgent; 
        mManager->allocateNode(NULL/*name*/,dataspec[i]/*dataSpec*/,observers[i]/*observer*/,&nodes[i]);
        LLOGD("node = %d",nodes[i]);
        mManager->prepareLoader(nodes[i]);
        mManager->startLoader(nodes[i]);
    }

        usleep(10000000ll);
        
    for(int i = 0;i < NODE_NUM; i++) {
        long long speed = 0ll;
        mManager->getConfig(DL_IndexComponentDownloadSpeedGet,&speed,sizeof(speed), nodes[i]);
        mManager->getConfig(DL_IndexComponentDownloadInfo,&info,sizeof(info), nodes[i]);
        LLOGD("node = %d url: %s  ip:  %s  download speed = %lld",nodes[i],info.effective_url,info.effective_ip,speed);
        LLOGD("node = %d effective_url: %lld effective_ip:  %lld ",nodes[i],info.effective_url,info.effective_ip);

        if(info.effective_url != NULL) {
            free((void*)(info.effective_url));
            info.effective_url = NULL;
        }

        if(info.effective_ip != NULL) {
            free((void*)(info.effective_ip));
            info.effective_ip = NULL;
        }

        if(info.content_type != NULL) {
            free((void*)(info.content_type));
            info.content_type = NULL;
        }
            
        mManager->stopLoader(nodes[i]);
        LLOGD("node = %d",nodes[i]);
        mManager->freeNode(nodes[i]);
        delete dataspec[i];
    }

    free(userAgent);
    free(tmp);

}


