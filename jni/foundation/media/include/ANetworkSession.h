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

#ifndef __LIYL_A_NETWORK_SESSION_H_

#define __LIYL_A_NETWORK_SESSION_H_

#include "ABase.h"
#include "../../system/core/utils/RefBase.h"
#include "../../system/core/utils/Thread.h"
#include <map>
#include <netinet/in.h>

LIYL_NAMESPACE_START

struct AMessage;

// Helper class to manage a number of live sockets (datagram and stream-based)
// on a single thread. Clients are notified about activity through AMessages.
struct ANetworkSession : public RefBase {
    ANetworkSession();

    status_t start();
    status_t stop();

    status_t createRTSPClient(
            const char *host, unsigned port, const sp<AMessage> &notify,
            int32_t *sessionID);

    status_t createRTSPServer(
            const struct in_addr &addr, unsigned port,
            const sp<AMessage> &notify, int32_t *sessionID);

    status_t createUDPSession(
            unsigned localPort, const sp<AMessage> &notify, int32_t *sessionID);

    status_t createUDPSession(
            unsigned localPort,
            const char *remoteHost,
            unsigned remotePort,
            const sp<AMessage> &notify,
            int32_t *sessionID);

    status_t connectUDPSession(
            int32_t sessionID, const char *remoteHost, unsigned remotePort);

    // passive
    status_t createTCPDatagramSession(
            const struct in_addr &addr, unsigned port,
            const sp<AMessage> &notify, int32_t *sessionID);

    // active
    status_t createTCPDatagramSession(
            unsigned localPort,
            const char *remoteHost,
            unsigned remotePort,
            const sp<AMessage> &notify,
            int32_t *sessionID);

    status_t destroySession(int32_t sessionID);

    status_t sendRequest(
            int32_t sessionID, const void *data, ssize_t size = -1,
            bool timeValid = false, int64_t timeUs = -1ll);

    status_t switchToWebSocketMode(int32_t sessionID);

    enum NotificationReason {
        kWhatError,
        kWhatConnected,
        kWhatClientConnected,
        kWhatData,
        kWhatDatagram,
        kWhatBinaryData,
        kWhatWebSocketMessage,
        kWhatNetworkStall,
    };

protected:
    virtual ~ANetworkSession();

private:
    struct NetworkThread;
    struct Session;

    Mutex mLock;
    sp<Thread> mThread;

    int32_t mNextSessionID;

    int mPipeFd[2];

    typedef std::map<int32_t, sp<Session> > tSessionsMap;
    typedef std::pair<int32_t, sp<Session> > tSessionsPair;
    tSessionsMap mSessions;

    enum Mode {
        kModeCreateUDPSession,
        kModeCreateTCPDatagramSessionPassive,
        kModeCreateTCPDatagramSessionActive,
        kModeCreateRTSPServer,
        kModeCreateRTSPClient,
    };
    status_t createClientOrServer(
            Mode mode,
            const struct in_addr *addr,
            unsigned port,
            const char *remoteHost,
            unsigned remotePort,
            const sp<AMessage> &notify,
            int32_t *sessionID);

    void threadLoop();
    void interrupt();

    static status_t MakeSocketNonBlocking(int s);

    DISALLOW_EVIL_CONSTRUCTORS(ANetworkSession);
};

LIYL_NAMESPACE_END

#endif  // __LIYL_A_NETWORK_SESSION_H_
