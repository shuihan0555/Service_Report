/*************************************************************************
	> File Name: main.cc
	> Author: xyz
	> Mail: xiao13149920@foxmail.com 
	> Created Time: Wed 22 Feb 2017 03:41:26 PM CST
 ************************************************************************/
#include<unistd.h>
#include<stdio.h>
#include<signal.h>
#include"tnode_adapter.h"
#include"ThreadPool.h"
//#include"position_list.h"
#include"mysqlop.h"
#include"object_pool.h"
#include"redisop.h"
#include"solution_config.h"
//#include"concurrentqueue.h"
#include"xdatetime.h"
//#include"position_regex_json.h"
#include<stdarg.h>

using namespace snetwork_xservice_xflagger;

TNode *g_tnode = nullptr;
ThreadPool* g_threadPool = nullptr;
using RedisPool = ObjectPool<CRedis, 5>;
RedisPool* g_redisPool = nullptr;
//CRedis *g_redis = nullptr;

using CMallocSizeNode = CMallocSize<char, CMALLOC_SIZE_TNODE>;
using CMallocSizePool = ObjectPool<CMallocSizeNode, 20>;
CMallocSizePool* g_cmallocSizePool = nullptr;

//typedef std::map<const char*, std::list<std::string>*> SymbolMap;//storge the coinpair base on symbol
//using SymbolMap = std::map<const char*, std::list<std::string>*> ;
//SymbolMap *g_symbolmap = new std::map<const char*, std::list<std::string>*>();
std::string g_symbolmap[1024];

std::string g_pmid;
ConnectionPool* g_connectionPool = nullptr;

void InitSymbolMap() {
    MySqlDB db;
    MySqlRecordSet rs;

    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sqlbuf = nullptr;
    asprintf(&sqlbuf, SELECT_FROM_TABLE_COIN);
    rs = db.QuerySql(sqlbuf);
    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("%s,con=%d|sql=%s\n", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
        db.Close();
    } else {
        db.Close();
    }

    size_t rows = rs.GetRows();
    if (rows == 0) {
#if defined(DEBUG)
        XWARNING("Empty result|sql=%s\n", sqlbuf);
#endif
        FREE(&sqlbuf);

        return;
    } else {
        FREE(&sqlbuf);
        //extern char *g_symbolmap[1024];
        for (int i = 0; i < rows; i++) {
            int index = atoi(rs.GetFieldByName(i, "ID").c_str());
            g_symbolmap[index] = rs.GetFieldByName(i, "CoinName");
//            std::string str[2];
//            str[1] = rs.GetFieldByName(i,"CoinName");
//            std::string str = rs.GetFieldByName(i, "CoinName");
//            std::cout << rs.GetFieldByName(i,"CoinName") << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
	signal(SIGPIPE, SIG_IGN);

	XConfig* cfg = new SXConfig;
	XFlagger* flagger = SXFlagger::GetInstance();
	flagger->ParseCommandLine(&argc, &argv, false);
	flagger->FlagXConfig(cfg);
	SXConfig* sxconfig = dynamic_cast<SXConfig*>(flagger->GetConfig());
	if (sxconfig == nullptr) {
		fprintf(stderr, "config error");

		exit(1);
	}
	std::cout<<*sxconfig<<std::endl;
	/* logger setting */

	std::string fileName (sxconfig->LoggerFile());
	if (!sxconfig->Screen()) {
		fileName = sxconfig->LoggerFile();
	}
	(*XLogger::GetObject(fileName.c_str())).StdErr(sxconfig->Screen()).Colour(sxconfig->Color()).Dir(sxconfig->LoggerDir());

    g_redisPool = RedisPool::GetInstance(sxconfig->RedisHost().c_str(),
                                         sxconfig->RedisPort(),
                                         sxconfig->RedisDB(),
                                         sxconfig->RedisAuth().c_str());
//    g_redis = new CRedis(sxconfig->RedisHost().c_str(),
//                         sxconfig->RedisPort(),
//                         sxconfig->RedisDB(),
//                         sxconfig->RedisAuth().c_str());

	g_cmallocSizePool = CMallocSizePool::GetInstance();
    g_connectionPool = ConnectionPool::GetInstance(sxconfig->MySqlHost().c_str(),
                                                                         sxconfig->MySqlUser().c_str(),
                                                                         sxconfig->MySqlPassword().c_str(),
                                                                         sxconfig->MySqlPort(),
                                                                         sxconfig->ConnSize());
    //init symbol
    InitSymbolMap();

    //for tnode
    STTnodeConfig tnodeconfig;
    tnodeconfig.mq_vhost = sxconfig->VHost();
    tnodeconfig.mq_exchange_group = sxconfig->ExchangeGroup();
    tnodeconfig.mq_host = sxconfig->Address();
    tnodeconfig.mq_port = atoi(sxconfig->Port().c_str());
    tnodeconfig.mq_user = sxconfig->User();
    tnodeconfig.mq_passwd = sxconfig->Password();

    g_tnode = new TNode(tnodeconfig);
    if (g_tnode->Init() == -1) {
        XERROR("tnode init error");

        exit(1);
    }

    //for thread
	g_threadPool = new ThreadPool(sxconfig->ThreadSize());
    g_pmid = sxconfig->SolutionID();


    Consumer *comsumer = new Consumer(WEB2REPORSERVER_BINDINGKEY_REQUEST, WEB2REPORSERVER_QUEUENAME_REQUEST);
    g_tnode->AddTNodeConsumer(comsumer);

//    char *bindingkey = nullptr;
//    char *queuename = nullptr;
//    asprintf(&bindingkey, REPORSERVER2WEB_BINDINGKEY_ANS, 1);
//    asprintf(&queuename, REPORSERVER2WEB_QUEUENAME_ANS, 1);
//    Consumer *comsumerans = new Consumer(bindingkey, queuename);
//    g_tnode->AddTNodeConsumer(comsumerans);
//    FREE(&bindingkey);
//    FREE(&queuename);
//    Consumer *consumerans = new Consumer(REPORSERVER2WEB_BINDINGKEY_ANS, REPORSERVER2WEB_QUEUENAME_ANS);
//    g_tnode->AddTNodeConsumer(consumerans);
    g_tnode->RunConsumer();

    while(true) {
        char *json =nullptr;
////        //history orders done(mysql
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"SessionID\":\"asdfadsf\",\"UserID\":10005,"
//                        "\"Symbol\":\"RKT/BTC\",\"OrderType\":0,\"BeginTime\":0,"
//                        "\"EndTime\":20180423173303,\"WSID\":1}", EHISTORYORDER);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        //MaxMin orders done(redis)//*
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"SessionID\":\"abcdef\","
//          "\"Symbol\":\"RKT/BTC\",\"WSID\":1}", EMAXMIN);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        //asset searching
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"SessionID\":\"abcdefg\","
//                        "\"UserID\":10000,\"WSID\":1}", EASSETRECORD);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

//        //pending orders//*
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"SessionID\":\"01234567890182\","
//                        "\"UserID\":10005,\"Symbol\":\"RKT/BTC\",\"ordertype\":0,"
//                        "\"begintime\":1517277000,\"endtime\":1517277000,\"limitedays\":15,"
//                        "\"WSID\":1}", EPENDINGORDER);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;
////        //pending orders//*
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"SessionID\":\"2333\","
//                        "\"UserID\":10005,\"Symbol\":\"RKT/BTC\",\"OrderType\":0,"
//                        "\"BeginTime\":1517277000,\"EndTime\":1517277000,\"WSID\":1}", EPENDINGORDER);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        //for rest balance done(mysql)
//        asprintf(&json, "{\"Tag\":%d,\"SessionID\":\"1542356134134\", \"RequestID\":\"asdfadsf\",\"UserID\":10005,"
//                        "\"BaseCoin\": \"ETH\", \"TradeCoin\":\"BTC\", \"WSID\":1}", EBALANCE);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        //login history done(mysql)
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"UserID\":10000,\"WSID\":1}", ELOGINHISTORY);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        //for realmarket
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"SessionID\":\"abcdefg\",\"UserID\":10005,"
//                        "\"WSID\":1,\"Symbol\":\"BTC\"}", EMAKETSRV);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        //for historymarket
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"SessionID\":\"abcdefg\",\"UserID\":10005,"
//                        "\"WSID\":1,\"Symbol\":\"RKT/BTC\",\"Timeframe\":\"M1\"}", EHISTORYMARKET);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        //for usersecurity
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"asdfadsf\",\"SessionID\":\"abcdefg\",\"UserID\":10000,\"WSID\":1}", EUSERSECURITY);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;
////        // for coincheck
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"abcdefghijk\",\"SessionID\":\"238528375012358\",\"WSID\":1}", ECOINCHECK);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        // for symbolcheck
//        asprintf(&json, "{\"Tag\":%d,\"RequestID\":\"abcdefghijk\",\"SessionID\":\"238528375012358\",\"Symbol\":\"All\","WSID\":1}", ESYMBOLCHECK);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

////        //for EORDERBOOK
//        asprintf(&json, "{\"Tag\":%d,\"Symbol\":\"BTC/RKT\",\"RequestID\":\"abcdefghijk\",\"SessionID\":\"238528375012358\",\"WSID\":1}", EORDERBOOK);
//        g_tnode->PublishToMQ(WEB2REPORTERVER_ROUTINGKEY_REQUEST, json, strlen(json)+1);
//        FREE(&json);
//        json = nullptr;

          sleep(1.5);
    }

    delete comsumer;
    //delete comsumerans;

	FREEXLOGGER;
	return 0;
}


