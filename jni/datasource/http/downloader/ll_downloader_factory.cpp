/****************************************************
 *
 *  @Author: liyl- liyunlong880325@gmail.com
 *  Last modified: 2017-05-26 14:17
 *  @Filename: vi
 *  @Description: 
 *  @param: 
 *  @return: 
 *****************************************************/

#include "ll_downloader_factory.h"
#include "ll_http_downloader.h"
#include "ll_test_downloader.h"

#undef LOG_TAG
#define LOG_TAG "DownloaderFactory"


LIYL_NAMESPACE_START

Mutex DownloaderFactory::sLock;
DownloaderFactory::tFactoryMap DownloaderFactory::sFactoryMap;
bool DownloaderFactory::sInitComplete = false;

status_t DownloaderFactory::registerFactory_l(IFactory* factory,
        downloader_type type) {
    if (NULL == factory) {
        LLOGE("Failed to register DownloaderFactory of type %d, factory is"
                " NULL.", type);
        return BAD_VALUE;
    }

    tFactoryMap::iterator it= sFactoryMap.find(type); 
    if (it != sFactoryMap.end()) {
        LLOGE("Failed to register DownloaderFactory of type %d, type is"
                " already registered.", type);
        return ALREADY_EXISTS;
    }
    if ((sFactoryMap.insert(std::pair<downloader_type,IFactory*>(type, factory))).second == false) {
        LLOGE("Failed to register DownloaderFactory of type %d, failed to add"
                " to map.", type);
        return UNKNOWN_ERROR;
    }

    return OK;
}

static downloader_type getDefaultDownloaderType() {
    return DL_TYPE_HTTP;
}

status_t DownloaderFactory::registerFactory(IFactory* factory,
        downloader_type type) {
    Mutex::Autolock lock(sLock);
    return registerFactory_l(factory, type);
}

void DownloaderFactory::unregisterFactory(downloader_type type) {
    Mutex::Autolock lock_(sLock);
	IFactory* factory = sFactoryMap[type];
	sFactoryMap.erase(type);
	delete factory;
}

#define GET_DOWNLOADER_TYPE_IMPL(a...)                      \
    Mutex::Autolock lock(sLock);                      \
\
downloader_type ret = DL_TYPE_HTTP;                 \
float bestScore = 0.0;                              \
\
for (size_t i = 0; i < sFactoryMap.size(); ++i) {   \
    \
    IFactory* v = sFactoryMap[(downloader_type)i];\
    float thisScore;                                \
    CHECK(v != NULL);                               \
    thisScore = v->scoreFactory(a, bestScore);      \
    if (thisScore > bestScore) {                    \
        ret = (downloader_type)i;                   \
        bestScore = thisScore;                      \
    }                                               \
}                                                   \
\
if (0.0 == bestScore) {                             \
    ret = getDefaultDownloaderType();               \
}                                                   \
\
return ret;

downloader_type DownloaderFactory::getDownloadType( const char* url) {
    GET_DOWNLOADER_TYPE_IMPL(url);
}




#undef GET_DOWNLOADER_TYPE_IMPL

sp<IDownloader> DownloaderFactory::createDownloader(
        downloader_type downloaderType,
        void* cookie,
        notify_callback_f notifyFunc,
        pid_t pid) {
    sp<IDownloader> d;
    IFactory* factory;
    status_t init_result;
    Mutex::Autolock lock_(sLock);

    for(tFactoryMap::iterator it  = sFactoryMap.begin();it != sFactoryMap.end();it++) {
		LLOGD("sFactoryMap type = %d ",it->first);
	}
    tFactoryMap::iterator it= sFactoryMap.find(downloaderType); 

    if (it == sFactoryMap.end()) {
        LLOGE("Failed to create downloader object of type %d, no registered"
                " factory", downloaderType);
        return d;
    }

    factory = sFactoryMap[downloaderType];
    CHECK(NULL != factory);
    d = factory->createDownloader(pid);

    if (d == NULL) {
        LLOGE("Failed to create downloader object of type %d, create failed",
                downloaderType);
        return d;
    }

    init_result = d->initCheck();
    if (init_result == NO_ERROR) {
        //d->setNotifyFun(cookie, notifyFunc);//TODO unused
    } else {
        LLOGE("Failed to create downloader object of type %d, initCheck failed"
                " (res = %d)", downloaderType, init_result);
        d->clear();
    }

    return d;
}

/*****************************************************************************
 *                                                                           *
 *                     Built-In Factory Implementations                      *
 *                                                                           *
 *****************************************************************************/

class HTTPDownloaderFactory : public DownloaderFactory::IFactory {
    public:
        virtual float scoreFactory( const char* url,
                float curScore) {
            static const float kOurScore = 0.8;

            if (kOurScore <= curScore)
                return 0.0;

            if (!strncasecmp("http://", url, 7)
                    || !strncasecmp("https://", url, 8)) {
                size_t len = strlen(url);
                if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) {
                    return kOurScore;
                }

                if (strstr(url,"m3u8")) {
                    return kOurScore;
                }

            }

            return 0.0;
        }


        virtual sp<IDownloader> createDownloader(pid_t pid) {
            LLOGV(" create HTTPDownloader");
			sp<IDownloader> ret = new HTTPDownloader();
			return ret;
        }
};

class TestDownloaderFactory: public DownloaderFactory::IFactory {
    public:
        virtual float scoreFactory( const char* url,
                float /*curScore*/) {
            /*if (TestDownloader::canBeUsed(url)) {
              return 1.0;
              }*/

            return 0.0;
        }

        virtual sp<IDownloader> createDownloader(pid_t /* pid */) {
            LLOGV("Create Test Downloader");
            //return new TestDownloader();
            return NULL;
        }
};

void DownloaderFactory::registerBuiltinFactories() {
    Mutex::Autolock lock_(sLock);

    registerFactory_l(new HTTPDownloaderFactory(), DL_TYPE_HTTP);
    //registerFactory_l(new TestDownloaderFactory(), DL_TYPE_TEST);

    sInitComplete = true;
}

LIYL_NAMESPACE_END
