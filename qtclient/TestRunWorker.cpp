

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

typedef void (*TestRun_live_cb)(int, uint32_t, int64_t);
typedef void (*TestRun_status_cb)(double, double);

TestRunWorker::TestRunWorker(QObject *parent,
                             struct TestRunConfig *tConfig,
                                     struct ListenConfig *listenConfig) :
        QObject(parent)
{
    //memcpy(&testRunConfig, tConfig, sizeof(testRunConfig));
    this->testRunConfig = tConfig;
    this->listenConfig = listenConfig;


    Callback<void(int, uint32_t, int64_t)>::func = std::bind(&TestRunWorker::testRunDataCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    TestRun_live_cb func = static_cast<TestRun_live_cb>(Callback<void(int, uint32_t, int64_t)>::callback);
    testRunConfig->TestRun_live_cb = func;

    Callback<void(double,double)>::func = std::bind(&TestRunWorker::testRunStatusCB, this, std::placeholders::_1, std::placeholders::_2);
    TestRun_status_cb status_cb = static_cast<TestRun_status_cb>(Callback<void(double, double)>::callback);
    testRunConfig->TestRun_status_cb = status_cb;

    //memcpy( &this->listenConfig, listenConfig, sizeof(struct ListenConfig));
    /* Setting up UDP socket  */
    setupSocket(this->listenConfig, this->listenConfig);

    initTestRunManager(&testRunManager);

}

TestRunWorker::~TestRunWorker()
{
   // mp = nullptr;
}



void TestRunWorker::testRunDataCb(int i, uint32_t seq, int64_t jitter)
{
    //cout<<"TestRun: "<<i<<"  seg:"<<seq<<"  jitter: "<<jitter<<endl;
    emit sendData(i, seq, jitter);
}

void TestRunWorker::testRunStatusCB(double mbps, double ps){
   emit sendPktStatus(mbps, ps);
}

void TestRunWorker::startTests()
{
    addTxAndRxTests(testRunConfig, &testRunManager, listenConfig);
    int sockfd = startListenThread(&testRunManager, listenConfig);
    runAllTests(sockfd, testRunConfig, &testRunManager, listenConfig);
    emit finished();

}

void TestRunWorker::stopTests()
{
    

    std::cout<<"Stopping tests.."<<std::endl;

}

