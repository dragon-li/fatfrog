/*************************************************************************
	> File Name: ll_http_downloader.h
	> Author: liyunlong
	> Mail: liyunlong_88@126.com 
	> Created Time: 2017年05月26日 星期五 15时15分15秒
 ************************************************************************/

#ifndef TESTDOWNLOADER
#define TESTDOWNLOADER

#include "ll_downloader.h"

LIYL_NAMESPACE_START

class TestDownloader :public IDownloader {
	protected:
	      virtual ~TestDownloader() {ENTER_FUNC;ESC_FUNC;}
	private:
		  DISALLOW_EVIL_CONSTRUCTORS(TestDownloader);
};
LIYL_NAMESPACE_END

#endif
