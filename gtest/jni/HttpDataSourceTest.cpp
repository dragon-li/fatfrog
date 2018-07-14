/*************************************************************************
  > File Name: HttpDataSourceTest.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2018年03月02日 星期五 11时08分56秒
 ************************************************************************/

#include "../../jni/datasource/http/HttpDataSource.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string>
#include <errno.h>
using namespace std;
USING_NAMESPACE_LIYL;

class HttpDataSourceTest: public ::testing::Test {
    public:
        virtual void SetUp(){
            ENTER_FUNC;
        }
        virtual void TearDown(){
            ESC_FUNC;
        }

    public:
};


TEST_F(HttpDataSourceTest, test0) { // startloading  unreachhost


    const int NODE_NUM = 2;
    char* tmp = (char*)malloc(512);
    sprintf(tmp,"%s","http://30.96.91.171/bbb_short.ffmpeg.1920x1080.mp4.libx264_10000kbps_30fps.libfaac_stereo_192kbps_48000Hz.mp4");
    sp<HttpDataSource> dataSource = new HttpDataSource(); 
    dataSource->open(tmp,NULL,NULL);
    for(int i = 0 ;i < NODE_NUM; i++) {
        dataSource->connectAtOffset(1000000,4000000);
        usleep(6000000ll);
        dataSource->disconnect();
        fprintf(stdout,"Queue size = %d \n",dataSource->countQueuedBuffers());
    }
    dataSource->close();
    free(tmp);

}

TEST_F(HttpDataSourceTest, test1) { // startloading 
    const int NODE_NUM = 2;
    char* tmp = (char*)malloc(512);
    sprintf(tmp,"%s","http://30.96.74.41/bbb_short.ffmpeg.1920x1080.mp4.libx264_10000kbps_30fps.libfaac_stereo_192kbps_48000Hz.mp4");
    sp<HttpDataSource> dataSource = new HttpDataSource(); 
    dataSource->open(tmp,NULL,NULL);
    for(int i = 0 ;i < NODE_NUM; i++) {
        dataSource->connectAtOffset(1000000,4000000);
        usleep(6000000ll);
        dataSource->disconnect();
        fprintf(stdout,"Queue size = %d \n",dataSource->countQueuedBuffers());
    }
    dataSource->close();
    free(tmp);


}

TEST_F(HttpDataSourceTest, test2) { // parallel  load
    const int NODE_NUM = 10;
    char* tmp = (char*)malloc(512);
    sprintf(tmp,"%s","http://30.96.74.41/bbb_short.ffmpeg.1920x1080.mp4.libx264_10000kbps_30fps.libfaac_stereo_192kbps_48000Hz.mp4");
    sp<HttpDataSource> dataSources[NODE_NUM] = {NULL}; 
    for(int i = 0 ;i < NODE_NUM; i++) {
		dataSources[i] = new HttpDataSource();
        dataSources[i]->open(tmp,NULL,NULL);
        dataSources[i]->connectAtOffset((i%6)*1000000,((i%3)+1)*1000000);
    }
    usleep(20000000ll);
    for(int i = 0 ;i < NODE_NUM; i++) {
        fprintf(stdout,"Queue size = %d \n",dataSources[i]->countQueuedBuffers());
        dataSources[i]->disconnect();
        dataSources[i]->close();
    }
    free(tmp);


}


