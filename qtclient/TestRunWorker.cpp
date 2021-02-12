

#include "TestRunWorker.h"

#include <iostream>
#include <chrono>
#include <thread>

#include "clientcommon.h"


using namespace std;
template <typename T>
struct Callback;

template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
    template <typename... Args>
    static Ret callback(Args... args) {
        return func(args...);
    }
    static std::function<Ret(Params...)> func;
};


template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

typedef void (*TestRun_jitter_cb)(int, uint32_t, int64_t);
typedef void (*TestRun_pktloss_cb)(int, uint32_t, uint32_t);
typedef void (*TestRun_status_cb)(double, double);

TestRunWorker::TestRunWorker(QObject *parent,
                             struct TestRunConfig *tConfig,
                                     struct ListenConfig *listenConfig) :
        QObject(parent)
{
    this->testRunConfig = tConfig;
    this->listenConfig = listenConfig;
    setupSocket(this->listenConfig);
    testRunManager = NULL;
}

TestRunWorker::~TestRunWorker()
{
    freeTestRunManager(&testRunManager);
}



void TestRunWorker::testRunDataCb(int i, uint32_t seq, int64_t jitter)
{
    emit sendData(i, seq, jitter);
}

void TestRunWorker::testRunPktLossCb(int i, uint32_t start, uint32_t end)
{
    emit sendPktLoss(i, start, end);
}

void TestRunWorker::testRunStatusCB(double mbps, double ps){
   emit sendPktStatus(mbps, ps);
}

void TestRunWorker::startTests()
{
    if(testRunManager != NULL){
        return;
    }

    freeTestRunManager(&testRunManager);
    testRunManager = newTestRunManager();


    Callback<void(double,double)>::func = std::bind(&TestRunWorker::testRunStatusCB, this, std::placeholders::_1, std::placeholders::_2);
    TestRun_status_cb status_cb = static_cast<TestRun_status_cb>(Callback<void(double, double)>::callback);
    testRunManager->TestRun_status_cb = status_cb;

    /* Setting up UDP socket  */
    setupSocket(listenConfig);

    Callback<void(int, uint32_t, int64_t)>::func = std::bind(&TestRunWorker::testRunDataCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    TestRun_jitter_cb func = static_cast<TestRun_jitter_cb>(Callback<void(int, uint32_t, int64_t)>::callback);


    Callback<void(int, uint32_t, uint32_t)>::func = std::bind(&TestRunWorker::testRunPktLossCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    TestRun_pktloss_cb pktLossFunc = static_cast<TestRun_pktloss_cb>(Callback<void(int, uint32_t, uint32_t)>::callback);

    addTxAndRxTests(testRunConfig, testRunManager, listenConfig, func, pktLossFunc, false);

    int sockfd = startListenThread(testRunManager, listenConfig);
    int numPkts = runAllTests(sockfd, testRunConfig, testRunManager, listenConfig);

    pruneLingeringTestRuns(testRunManager);
    freeTestRunManager(&testRunManager);

    listenConfig->running = false;
    pthread_join(listenConfig->socketListenThread, nullptr);

    emit testRunFinished(numPkts);

}

void TestRunWorker::stopTests()
{
    testRunManager->done = true;
}

