/*
 * ll_downloader_factory.h
 *
 *  Created on: 17年5月23日
 *      Author: wangqianpeng
 */

#ifndef LL_DOWNLOADER_FACTORY_H
#define LL_DOWNLOADER_FACTORY_H

#include <string>
#include <vector>
#include <map>


#include "error.h"
#include "ll_downloader.h"


using std::string;
using std::vector;
using std::map;

LIYL_NAMESPACE_START

class DownloaderFactory {
    public:
        class IFactory {
            public:
                virtual ~IFactory() { }

                virtual float scoreFactory(
                        const char* /*url*/,
                        float /*curScore*/) { return 0.0; }

                virtual sp<IDownloader> createDownloader(pid_t pid) = 0;
        };

        static status_t registerFactory(IFactory* factory,
                downloader_type type);
        static void unregisterFactory(downloader_type type);
        static downloader_type getDownloadType(const char* url);
        static sp<IDownloader> createDownloader(downloader_type downloaderType,
                void* cookie,
                notify_callback_f notifyFunc,
                pid_t pid);

        static void registerBuiltinFactories();
    protected:
        virtual ~DownloaderFactory(){}

    private:
        typedef std::map<downloader_type, IFactory*> tFactoryMap;


        DownloaderFactory() { }
        static status_t registerFactory_l(IFactory* factory,
                downloader_type type);

        static Mutex       sLock;
        static tFactoryMap sFactoryMap;
        static bool        sInitComplete;

        DISALLOW_EVIL_CONSTRUCTORS(DownloaderFactory);
};

LIYL_NAMESPACE_END
#endif /* LL_DOWNLOADER_FACTORY_H_ */
