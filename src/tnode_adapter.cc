/*************************************************************************
	> File Name: tnode_adapter.cc
	> Author: xyz
	> Mail: xiao13149920@foxmail.com 
	> Created Time: Wed 08 Feb 2017 11:17:29 AM CST
 ************************************************************************/
#include<mutex>
#include<new>
#include<string.h>
#include<type_traits>
#include<stdio.h>
#include"tnode_adapter.h"
#include"ThreadPool.h"
#include<rapidjson/document.h>
#include<rapidjson/writer.h>
#include<rapidjson/rapidjson.h>
#include<rapidjson/stringbuffer.h>
#include"object_pool.h"
#include"tnode.h"
#include"mysqlop.h"
#include"redisop.h"
#include<regex.h>
#include <sys/types.h>
#include<map>
#include<list>

#if defined(DPOSITION)
#include"solution_config.h"
//#include"reportsvr_subscribe.h"
#endif
extern ThreadPool* g_threadPool;
using RedisPool = ObjectPool<CRedis, 5>;
extern RedisPool* g_redisPool;
//extern CRedis *g_redis;

using CMallocSizeNode = CMallocSize<char, CMALLOC_SIZE_TNODE>;
using CMallocSizePool = ObjectPool<CMallocSizeNode, 20>;
extern CMallocSizePool* g_cmallocSizePool;

//typedef std::map<const char*, std::list<std::string>*> SymbolMap;//storge the coinpair base on symboll
//using SymbolMap = std::map<const char*, std::list<std::string>*> ;
//extern SymbolMap *g_symbolmap;
extern std::string g_symbolmap[];

extern TNode *g_tnode;
using namespace snetwork_xservice_xflagger;
using namespace snetwork_xservice_tnode;
#define DATA_PACKET_SIZE 10
#define HISTORY_MARKET_PAKET_LEN 5
#define MARKET_DB_INDEX 1
#define ORDERBOOK_DB_INDEX 4

/*Subscribe   begin ****/
Subscribe::Subscribe(Consumer *consumer) {
    //consumer
    m_consumer = consumer;
}

Subscribe::Subscribe(const Subscribe& r) {
    m_consumer = r.m_consumer;
}

Subscribe::Subscribe(Subscribe&& r) {
	//printf("+++++++++++++++++++\n");
    m_consumer = r.m_consumer;
    r.m_consumer = nullptr;
}
/*Subscribe   end ****/


Consumer::Consumer(const char *bindingkey, const char *queuename) :
    m_bindingkey(bindingkey),
    m_queuename(queuename){

    SetupMapRequest();
}

void Consumer::CreateSubscibe(void) {

}
//1
void Consumer::SubscribeHistoryOrderRequest(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    /* Parse Json */
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parser json error|event=%s", event);
#endif
        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if(((it = d.FindMember("UserID")) == d.MemberEnd()) || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    int userid = it->value.GetInt();


    if(((it = d.FindMember("WSID")) == d.MemberEnd()) || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    int wsid = it->value.GetInt();

    if(((it = d.FindMember("SessionID")) == d.MemberEnd()) || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    const char* sessionid = it->value.GetString();


    if(((it = d.FindMember("Symbol")) == d.MemberEnd()) || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    const char* symbol = it->value.GetString();

    if(((it = d.FindMember("BeginTime")) == d.MemberEnd()) || !it->value.IsInt64()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    long  begintime = it->value.GetInt64();

    if(((it = d.FindMember("EndTime")) == d.MemberEnd()) || !it->value.IsInt64()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    long endtime= it->value.GetInt64();

    MySqlRecordSet rs_position;
    MySqlDB db;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sqlbuf = nullptr;
    //query by different conditions
    if(strcmp(symbol, "All") == 0) {
        if (begintime | endtime) {

            if (endtime == 0) {
                 asprintf(&sqlbuf, "Select * from `token_order` where UserID = %d and LastTraderTime >= %d order by LastTraderTime desc",
                     userid, begintime);
            } else{

                 asprintf(&sqlbuf, "Select * from `token_order` where UserID = %d and LastTraderTime >= %d and LastTraderTime <= %d order by LastTraderTime desc",
                     userid, begintime, endtime);
            }
        } else {
            asprintf(&sqlbuf, "Select * from `token_order` where UserID = %d order by LastTraderTime desc",userid);
        }
    } else {
        if (begintime | endtime) {

            if (endtime == 0) {
                 asprintf(&sqlbuf, "Select * from `token_order` where UserID = %d and Symbol = '%s' and LastTraderTime >= %d order by LastTraderTime desc ",
                     userid, symbol, begintime);
            } else {

                 asprintf(&sqlbuf, "Select * from `token_order` where UserID = %d and Symbol = '%s' and LastTraderTime >= %d and LastTraderTime <= %d order by LastTraderTime desc ",
                     userid, symbol, begintime, endtime);
            }
        } else {
            asprintf(&sqlbuf, "Select * from `token_order` where UserID = %d and Symbol = '%s' order by LastTraderTime desc",userid, symbol);
        }
    }

    rs_position = db.QuerySql(sqlbuf);
    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("%s,con=%d|sql=%s\n", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
        db.Close();
    } else {
        db.Close();
    }

    size_t rows = rs_position.GetRows();

    char* prefix = nullptr;
    asprintf(&prefix, "{\"Tag\":%d,\"UserID\":%d,\"SessionID\":\"%s\",\"WSID\":%d, "
                      "\"OrderList\":[", EHISTORYORDERANS, userid, sessionid, wsid);
    std::string jsonRespone;
    jsonRespone.append(prefix);
    FREE(&prefix);

    if (rows == 0) {
#if defined(DEBUG)
        XWARNING("Empty result|sql=%s\n", sqlbuf);
#endif

        FREE(&sqlbuf);

        jsonRespone.insert(jsonRespone.length(), 1, ']');
        jsonRespone.append("}");
    } else {
        FREE(&sqlbuf);
#if 0 //divice into mutilple pages
        int k = rows/DATA_PACKET_SIZE + 1;
        for(int l = 0; l < k; l++) {
            char* prefix = nullptr;
            asprintf(&prefix, "{\"Tag\":%d,\"UserID\":%d,\"SessionID\":\"%s\",\"WSID\":%d, \"OrderList\":[", EHISTORYORDERANS, userid, sessionid, wsid);
            std::string jsonRespone;
            jsonRespone.append(prefix);
            FREE(&prefix);

            for(int j = 0; j < DATA_PACKET_SIZE && rows !=0; j++) {//coulde be optimized, this 'if' can replace the out layer determination
                OrderQueryAns orderqueryans;
                orderqueryans.Id = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "ID").c_str());
                orderqueryans.UserId = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "UserID").c_str());
                orderqueryans.DelegateType = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "DelegateType").c_str());
                orderqueryans.TransType = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "Type").c_str());
                orderqueryans.ExcelID = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "ExcelID").c_str());
                orderqueryans.OrderIDFrME = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "OrderIDFrME").c_str());
                orderqueryans.Status = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "Status").c_str());
                orderqueryans.Price = atof(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "Price").c_str());
                orderqueryans.Amount = atof(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "Amount").c_str());
                orderqueryans.TradeAmount = atof(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "TradeAmount").c_str());
                orderqueryans.AveragePrice = atof(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "AveragePrice").c_str());
                orderqueryans.orderTime = atol(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "OrderTime").c_str());
                orderqueryans.LastTraderTime = atol(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "LastTraderTime").c_str());
                orderqueryans.CancelTime = atol(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "CancelTime").c_str());

                char* msg_orderqueryans = const_cast<char*>(orderqueryans.ToJson());


                jsonRespone.append(msg_orderqueryans);
                jsonRespone.append(",");

                rows--;
            }

//            jsonRespone[jsonRespone.length() - 1] = ']';
            jsonRespone.replace(jsonRespone.length() - 1, 1, "]");
            jsonRespone.append("}");

#if defined(DEBUG)
//          XINFO("<orderqueryans>:<%s>, len=%d", jsonRespone.c_str(), jsonRespone.length());
#endif
            char *routingkey = nullptr;
            asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
            g_tnode->PublishToMQ(routingkey, const_cast<char*>(jsonRespone.c_str()), jsonRespone.length()+1);
            FREE(&routingkey);
        }
#endif
            int len = DATA_PACKET_SIZE;
            if (rows < len) {
                len = rows;
            }

            for(int i = 0; i < len; i++) {//coulde be optimized, this 'if' can replace the out layer determination
                OrderQueryAns orderqueryans;
                orderqueryans.Id = rs_position.GetFieldByName(i, "ID");
                orderqueryans.UserId = atoi(rs_position.GetFieldByName(i, "UserID").c_str());
                orderqueryans.Symbol = rs_position.GetFieldByName(i, "Symbol");

                orderqueryans.DelegateType = atoi(rs_position.GetFieldByName(i, "DelegateType").c_str());
                orderqueryans.TransType = atoi(rs_position.GetFieldByName(i, "Type").c_str());
                orderqueryans.ExcelID = atoi(rs_position.GetFieldByName(i, "ExcelID").c_str());
                orderqueryans.OrderIDFrME = atoi(rs_position.GetFieldByName(i, "OrderIDFrME").c_str());
                orderqueryans.Status = atoi(rs_position.GetFieldByName(i, "Status").c_str());
                orderqueryans.Price = atof(rs_position.GetFieldByName(i, "Price").c_str());
                orderqueryans.Amount = atof(rs_position.GetFieldByName(i, "Amount").c_str());
                orderqueryans.TradeAmount = atof(rs_position.GetFieldByName(i, "TradeAmount").c_str());
                orderqueryans.AveragePrice = atof(rs_position.GetFieldByName(i, "AveragePrice").c_str());
                orderqueryans.orderTime = atol(rs_position.GetFieldByName(i, "OrderTime").c_str());
                orderqueryans.LastTraderTime = atol(rs_position.GetFieldByName(i, "LastTraderTime").c_str());

                char* msg_orderqueryans = const_cast<char*>(orderqueryans.ToJson());


                jsonRespone.append(msg_orderqueryans);
                jsonRespone.append(",");
            }

            jsonRespone.replace(jsonRespone.length() - 1, 1, "]");
            jsonRespone.append("}");
        }

        char *routingkey = nullptr;
        asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
        g_tnode->PublishToMQ(routingkey, const_cast<char*>(jsonRespone.c_str()), jsonRespone.length()+1);

#if defined(DEBUG)
        XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, jsonRespone.c_str(), routingkey);
#endif
        FREE(&routingkey);
}

//2
void Consumer::SubscribePendingOrderRequest(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
      XERROR("parse json error|event=%s", event);
#endif

      return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if ((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    const char* sessisonid = it->value.GetString();

    if ((it = d.FindMember("RequestID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    const char* requestid = it->value.GetString();

    if ((it = d.FindMember("UserID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    int userid = it->value.GetInt();

    if ((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    int wsid = it->value.GetInt();

    if ((it = d.FindMember("Symbol")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    const char* symbolquery = it->value.GetString();

    if ((it = d.FindMember("BeginTime")) == d.MemberEnd() || !it->value.IsInt64()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    long begintime = it->value.GetInt64();

    if ((it = d.FindMember("EndTime")) == d.MemberEnd() || !it->value.IsInt64()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    long endtime = it->value.GetInt64();

    CRedis *redis = g_redisPool->GetObject();
    shared_ptr<redisReply> ry;
    if(redis->SelectDB(2)) {
        ry =  redis->Excute("SMEMBERS %d", userid);
        redis->SelectDB(0);
    }

    char *tmp = nullptr;
    asprintf(&tmp, "{\"Tag\":%d,\"UserID\":%d,\"IsEnd\":%d,\"SessionID\":\"%s\","
                   "\"RequestID\":\"%s\",\"SearchTradings\":[",
             EPENDINGORDERANS, userid, 0, sessisonid, requestid);
    string pendingorder(tmp);
    FREE(&tmp);
    tmp = nullptr;

    if (ry.get() != nullptr && ry.get()->type == REDIS_REPLY_ARRAY && ry.get()->elements != 0) {
        int len = 10;
        if (len > ry.get()->elements) {
            len = ry.get()->elements;
        }
        int cnt = 0;
        for (int i = 0; i < len; i++) {
            if (ry.get()->element[ry.get()->elements-i-1]->type == REDIS_REPLY_STRING) {
                char *strorderid = ry.get()->element[ry.get()->elements-i-1]->str;
                //std::cout << strorderid << std::endl;
                //long orderid = atol(strorderid);
                std::string symbol = redis->HGet(strorderid, "Symbol");
                int ordertype = atoi(redis->HGet(strorderid, "OrderType").c_str());
                int transtype = atoi(redis->HGet(strorderid, "TransType").c_str());
                float ordernumber = atof(redis->HGet(strorderid, "OrderNumber").c_str());//changed to orderamount at proper time
                float price = atof(redis->HGet(strorderid, "Price").c_str());
                long ordertime = atol(redis->HGet(strorderid, "OrderTime").c_str());
                int orderstatus = atoi(redis->HGet(strorderid, "OrderStatus").c_str());//status:1
        //        int excelid = atoi(redis->HGet(strorderid, "ExcelID").c_str());
        //        long orderidfrme = atol(redis->HGet(strorderid, "OrderIDFrME").c_str());
                long transacttime = atol(redis->HGet(strorderid, "TransactTime").c_str());
                float remainnum = atof(redis->HGet(strorderid, "RemainNum").c_str());//changed to remainamount at proper time
                float total = atof(redis->HGet(strorderid, "Total").c_str());

                if (strcmp(symbolquery, "All") != 0) {
                    if (symbol.compare(symbolquery) != 0) {
                        continue;
                    }
                }
                if (begintime | endtime) {
                    if (endtime == 0) {
                        if (begintime > ordertime) {
                            continue;
                        }
                    } else {
                        if (begintime > ordertime || endtime < ordertime) {
                            continue;
                        }
                    }
                }

                asprintf(&tmp, "{\"OrderID\":\"%s\",\"Time\":%ld,\"Symbol\":\"%s\",\"OrderType\":%d,\"TransType\":%d,"
                               "\"Price\":%f,\"Amount\":%f,\"Status\":%d},", strorderid,
                               ordertime, symbol.c_str(), ordertype, transtype, price, remainnum, orderstatus);
                pendingorder.append(tmp);
                FREE(&tmp);

                cnt++;
            }
        }

        if (cnt !=0) {

            pendingorder.replace(pendingorder.size() - 1, 1, "]");
            pendingorder.append("}");
        } else {

            pendingorder.insert(pendingorder.size(), 1, ']');
            pendingorder.append("}");
        }
    } else {
        pendingorder.insert(pendingorder.size(), 1, ']');
        pendingorder.append("}");
    }

    char* routingkey = nullptr;
    asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
    g_tnode->PublishToMQ(routingkey, const_cast<char*>(pendingorder.c_str()), pendingorder.size()+1);
#if defined(DEBUG)
        XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, pendingorder.c_str(), routingkey);
#endif
    FREE(&routingkey);

    g_redisPool->ReleaseObject(redis);
}
//3
void Consumer::SubscribeAssetRequest(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parser json error|event=%s", event);
#endif
        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    const char* sessionid = it->value.GetString();

    if((it = d.FindMember("UserID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    int userid = it->value.GetInt();

    if((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    int wsid = it->value.GetInt();

    MySqlRecordSet asset_respone;
    MySqlDB db;
    db.Open();

    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sqlbuf = nullptr;

//    asprintf(&sqlbuf, SELECT_FROM_TABLE_USER_ASSETS, userid);
    asprintf(&sqlbuf, SELECT_FROM_TABLE_USER_ASSETS, userid);
    asset_respone =  db.QuerySql(sqlbuf);
    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("%s, con=%d|sql=%s", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
        db.Close();
    } else {
        db.Close();
    }

    size_t rows = asset_respone.GetRows();
    if (rows == 0) {
#if defined(DEBUG)
        XINFO("Empty result|sql=%s\n", sqlbuf);
#endif
        FREE(&sqlbuf);

        return;
    } else {
        FREE(&sqlbuf);

        int k = rows/DATA_PACKET_SIZE + 1;

        for (int l = 0; l < k; l++) {
            char *tmp = nullptr;
            asprintf(&tmp, "{\"Tag\":%d,\"UserID\":%d,\"SessionID\":\"%s\",\"SearchAsserts\":[", EASSETRECORDANS, userid, sessionid);
            string str_asset_rep(tmp);
            FREE(&tmp);
            //just think about less than 15 records;
            for (int i = 0; i < DATA_PACKET_SIZE && 0 != rows ; i++) {
//                std::string coinname = asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "CoinName");
//                float total = atof(asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "Total").c_str());
//                float avaiable = atof(asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "Available").c_str());
//                float frozen = atof(asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "Frozen").c_str());
//                float valuation = atof(asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "Valuation").c_str());
//                asprintf(&tmp, "{\"CoinName\":\"%s\",\"Total\":%f,\"Available\":%f,"
//                               "\"Frozen\":%f,\"Valuation\":%f},",coinname.c_str(), total, avaiable, frozen, valuation);
                std::string coinname = asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "CoinName");
                std::string total = asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "Total");
                std::string avaiable = asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "Available");
                std::string frozen = asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "Frozen");
                std::string valuation = asset_respone.GetFieldByName(l*DATA_PACKET_SIZE +i, "Valuation");
                asprintf(&tmp, "{\"CoinName\":\"%s\",\"Total\":%s,\"Available\":%s,"
                               "\"Frozen\":%s,\"Valuation\":%s},",coinname.c_str(), total.c_str(), avaiable.c_str(),
                         frozen.c_str(), valuation.c_str());

                str_asset_rep.append(tmp);
                FREE(&tmp);

                rows--;
            }
            str_asset_rep.replace(str_asset_rep.size() - 1, 1, "]");
            str_asset_rep.append("}");

            char *routingkey = nullptr;
            asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
            g_tnode->PublishToMQ(routingkey, const_cast<char*>(str_asset_rep.c_str()), str_asset_rep.size()+1);
#if defined(DEBUG)
            XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, str_asset_rep.c_str(), routingkey);
#endif
            FREE(&routingkey);
        }
    }
}
#if 0
//4
void Consumer::SubscribeSymbolCheckRequest(char* event, unsigned int eventLen) {
#if defined(DEBUG)
    XINFO("msg:%s, len:%d\n", event, eventLen);
#endif
    /* Parse Json */
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parser json error|event=%s", event);
#endif
        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if((it = d.FindMember("UserID")) == d.MemberEnd()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    int userid = it->value.GetInt();

    if((it = d.FindMember("Symbol")) == d.MemberEnd()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    const char *symbol = it->value.GetString();

    if((it = d.FindMember("Typed")) == d.MemberEnd()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    int typed = it->value.GetInt();

    if((it = d.FindMember("WSID")) == d.MemberEnd()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    int wsid = it->value.GetInt();


    if (typed == 0)//means init
    {
//        1涨跌幅列表
//        2最值信息
        if (userid == 0) {
//        3历史记录(pending/traded order) 浏览用户不需要
        }
//        4 订单薄
//        5,历史行情
    } else {
//        最值信息
//        订单薄
//        历史行情
    }

}
//5

void Consumer::SubscribePriceLimitRequest(char* event, unsigned int eventLen) {
#if defined(DEBUG)
    XINFO("msg=<%s>, len=%d", event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parse json error|event=%s",event);
#endif

        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if((it = d.FindMember("WSID")) == d.MemberEnd()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    int wsid = it->value.GetInt();


    MySqlRecordSet rs_position;
    MySqlDB db;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sqlbuf = nullptr;
//    const char* SELECT_FROM_TABLE_COIN = "SELECT ID, CoinName FROM coin";
    asprintf(&sqlbuf, SELECT_FROM_TABLE_COIN);
    rs_position = db.QuerySql(sqlbuf);
    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("%s,con=%d|sql=%s", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
        db.Close();
    } else {
        db.Close();
    }

    size_t rows = rs_position.GetRows();
    if (rows == 0) {
#if defined(DEBUG)
        XWARNING("Empty result|sql=%s", sqlbuf);
#endif
        FREE(&sqlbuf);

        return;
    } else {
        FREE(&sqlbuf);
        string coinmap[1024];
        for (int i = 0; i < rows; i++) {
            int index = atoi(rs_position.GetFieldByName(i, "ID").c_str());
            coinmap[index] = rs_position.GetFieldByName(i, "CoinName");
        }
        //const char* SELECT_FROM_TABLE_SYSMBOL = "SELECT ID ,BaseCoinID, TradeCoinID FROM sysmbol ORDER BY BaseCoinID";
        asprintf(&sqlbuf, SELECT_FROM_TABLE_SYSMBOL);
        rs_position = db.QuerySql(sqlbuf);
        if (db.IsError()) {
#if defined(DEBUG)
            XERROR("%s, con=%d|sql=%s", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
            db.Close();
        } else {
            db.Close();
        }

        size_t rows = rs_position.GetRows();
        std::string pricelimitjson;
        if(rows ==0) {
#if defined(DEBUG)
            XWARNING("Empty result|sql=%s", sqlbuf);
#endif
            FREE(&sqlbuf);

            return;
        } else {
            FREE(&sqlbuf);
            char* tmp = nullptr;
            asprintf(&tmp, "{\"Tag\":%d, \"SymbolList\":[", EPRICERANGEANS);
            pricelimitjson = tmp;
            FREE(&tmp);

            for (int i = 0; i < rows; i++) {
                int symbolid = atoi(rs_position.GetFieldByName(i, "ID").c_str());
                int basecoinid = atoi(rs_position.GetFieldByName(i, "BaseCoinID").c_str());
                int tradecoinid= atoi(rs_position.GetFieldByName(i, "TradeCoinID").c_str());
                int updown = 0;
                asprintf(&tmp, "{\"BaseCoin\":%s, \"Symbol\":\"%s/%s\",\"SymbolID\":%d, \"UPDown\":%d},",
                         coinmap[basecoinid].c_str(),
                         coinmap[tradecoinid].c_str(),
                         coinmap[basecoinid].c_str(),
                         symbolid,
                         updown);
                pricelimitjson.append(tmp);
                FREE(&tmp);
            }

            pricelimitjson[pricelimitjson.size()-1] = ']';
            pricelimitjson.append("}");
        }

        char *routingkey = nullptr;
        asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
        g_tnode->PublishToMQ(REPORSERVER2WEB_ROUTINGKEY_ANS, const_cast<char*>(pricelimitjson.c_str()), pricelimitjson.size()+1);
        FREE(&routingkey);
    }

}
#endif
//6
void Consumer::SubscribeOrderBookRequest(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
      XERROR("parse json error|event=%s", event);
#endif

      return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if ((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    const char* sessisonid = it->value.GetString();

    if ((it = d.FindMember("RequestID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    const char* requestid = it->value.GetString();

    if ((it = d.FindMember("Symbol")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    const char* symbol = it->value.GetString();

    if ((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif

        return;
    }
    int wsid = it->value.GetInt();
    //change db befor operation and recover after operated.
    CRedis *redis = g_redisPool->GetObject();
    if (!redis->SelectDB(ORDERBOOK_DB_INDEX)) {
#if defined(DEBUG)
        XERROR("select db error");
#endif
        g_redisPool->ReleaseObject(redis);

        return;
    }

    std::shared_ptr<redisReply> ry = redis->Excute("GET %s", symbol);
//    redis->Excute("SET %s %s", symbol, strorderbook);
    if (ry.get()->type == REDIS_REPLY_ERROR) {
#if defined(DEBUG)
        XERROR("get redis error, err=<%s>", ry.get()->str);
#endif
        redis->SelectDB(0);
        g_redisPool->ReleaseObject(redis);
        return;
    }
    //....
    char* tmp = nullptr;
    asprintf(&tmp, "{\"Tag\":%d,\"RequestID\":\"%s\",\"SessionID\":\"%s\",\"WSID\":%d,\"Symbol\":\"%s\","
                           "\"List\":", EORDERBOOKANS, requestid, sessisonid, wsid, symbol);
    std::string ebookres(tmp);
    FREE(&tmp);

    if (ry.get()->type == REDIS_REPLY_NIL) {
        ebookres.append("{}");
    } else {
        ebookres.append(ry.get()->str, ry.get()->len);
    }
    ebookres.append("}");

    redis->SelectDB(0);
    g_redisPool->ReleaseObject(redis);

    char* routingkey = nullptr;
    asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
    g_tnode->PublishToMQ(routingkey, const_cast<char*>(ebookres.c_str()), ebookres.size()+1);
#if defined(DEBUG)
    XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, ebookres.c_str(), routingkey);
#endif
    FREE(&routingkey);
}
//7
void Consumer::SubscribeMaxMinValue(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    /* Parse Json */
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parser json error|event=%s", event);
#endif
        return;
    }
    rapidjson::Value::ConstMemberIterator it;
//    if((it = d.FindMember("UserID")) == d.MemberEnd()) {
//#if defined(DEBUG)
//        XERROR("field doesn't exsit!");
//#endif
//        return;
//    }
//    int userid = it->value.GetInt();


    if((it = d.FindMember("Symbol")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    const char* symbol = it->value.GetString();


    if((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    const char* sessionid = it->value.GetString();

    if((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit!");
#endif
        return;
    }
    int wsid = it->value.GetInt();

    CRedis *redis = g_redisPool->GetObject();
//    CRedis *redis = g_redis;
    if (!redis->SelectDB(MARKET_DB_INDEX)) {
#if defined(DEBUG)
        XERROR("select db error");
#endif
        g_redisPool->ReleaseObject(redis);

        return;
    }

    if (0 == redis->ExistsKey(symbol)) {
#if defined(DEBUG)
        XERROR("key %s doesn't exist", symbol);
#endif
//because of front end need empty data;
//        redis->SelectDB(0);
//        g_redisPool->ReleaseObject(redis);
//        return;
    }

    char* jsonRespone = nullptr;
//    std::string strcurrent = redis->HGet(symbol, "current");
    std::string stryesterday = redis->HGet(symbol, "yesterday");
    if (stryesterday.empty()) {
        stryesterday = "{}";
    }
    std::string strtoday = redis->HGet(symbol, "today");
    if (strtoday.empty()) {
        strtoday = "{}";
    }
//    asprintf(&reponsejson, "{\"Tag\":%d,\"UserID\":%d,\"Symbol\":\"%s\","
//                           "\"SessionID\":\"%s\", \"Current\":%f, \"Yesterday\":%s,"
//                           "\"Today\":%s}",EMAXMINANS, userid, symbol, sessionid,
//             atof(strcurrent.c_str()), stryesterday.c_str(), strtoday.c_str());

//    asprintf(&reponsejson, "{\"Tag\":%d,\"Symbol\":\"%s\","
//                           "\"SessionID\":\"%s\", \"Current\":%f, \"Yesterday\":%s,"
//                           "\"Today\":%s}",EMAXMINANS, symbol, sessionid,
//             atof(strcurrent.c_str()), stryesterday.c_str(), strtoday.c_str());

    asprintf(&jsonRespone, "{\"Tag\":%d,\"Symbol\":\"%s\","
                           "\"SessionID\":\"%s\", \"Yesterday\":%s,"
                           "\"Today\":%s}",EMAXMINANS, symbol,
             sessionid, stryesterday.c_str(), strtoday.c_str());
    char* routingkey = nullptr;
    asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
    g_tnode->PublishToMQ(routingkey, jsonRespone, strlen(jsonRespone)+1);
#if defined(DEBUG)
   XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, jsonRespone, routingkey);
#endif
    FREE(&jsonRespone);
    jsonRespone = nullptr;
    FREE(&routingkey);
    routingkey = nullptr;

    redis->SelectDB(0);
    g_redisPool->ReleaseObject(redis);
}

void Consumer::SubscribeRealMarket(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parse json error");
#endif
        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if ((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif

        return;
    }
    const char* sessionid = it->value.GetString();

    if ((it = d.FindMember("RequestID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif

        return;
    }
    const char* requestid = it->value.GetString();

//    if ((it = d.FindMember("UserID")) == d.MemberEnd()) {
//#if defined(DEBUG)
//        XERROR("field doesn't exist");
//#endif

//        return;
//    }
//    int userid = it->value.GetInt();

    if ((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif

        return;
    }
    int wsid = it->value.GetInt();

    if ((it = d.FindMember("Symbol")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif

        return;
    }
    const char* symbol = it->value.GetString();

    MySqlDB db;
    MySqlRecordSet rs;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sqlbuf = nullptr;
    asprintf(&sqlbuf, SELECT_FROM_TABLE_SYMBOL_WITH_BASECOINNAME, symbol);
    rs = db.QuerySql(sqlbuf);

    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("%s,con=%d|sql=%s", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
        db.Close();
    } else {
        db.Close();
    }

    size_t rows = rs.GetRows();
    if (rows ==0) {
#if defined(DEBUG)
            XWARNING("Empty result|sql=%s", sqlbuf);
#endif
        FREE(&sqlbuf);

        return;
    } else {
        FREE(&sqlbuf);

        char *tmp = nullptr;
//        asprintf(&tmp, "{\"Tag\":%d,\"RequestID\":\"%s\",\"SessionID\":\"%s\",\"UserID\":%d,\"WSID\":%d,"
//                               "\"SymbolList\":[", EMAKETSRVANS, requestid, sessionid, userid, wsid);
        asprintf(&tmp, "{\"Tag\":%d,\"RequestID\":\"%s\",\"SessionID\":\"%s\",\"WSID\":%d,"
                               "\"SymbolList\":[", EMAKETSRVANS, requestid, sessionid, wsid);
        std::string realmarkans(tmp);
        FREE(&tmp);

//        int cnt = rows;
        for (int i = 0; i < rows; i++) {
            CRedis *redis = g_redisPool->GetObject();
            redis->SelectDB(MARKET_DB_INDEX);
            int symbolid = atoi(rs.GetFieldByName(i, "ID").c_str());
            int basecoinid = atoi(rs.GetFieldByName(i,"BaseCoinID").c_str());
            int tradecoinid = atoi(rs.GetFieldByName(i, "TradeCoinID").c_str());
            //std::string coinname = rs.GetFieldByName(i, "CoinName");

            char *symbolpair = nullptr;
            asprintf(&symbolpair, "%s/%s", g_symbolmap[tradecoinid].c_str(), g_symbolmap[basecoinid].c_str());
            std::string real = redis->HGet(symbolpair, "today");
            redis->SelectDB(0);
            g_redisPool->ReleaseObject(redis);
            //FREE(&symbolpair);

            float changed = 0.0;
            if (real.empty()) {
        #if defined(DEBUG)
              XERROR("key %s doesn't exist, please check this db redis, updown default 0.0", symbolpair);
        #endif
//              cnt--;
//              continue;
            } else {
                //parse today json
                //d.Clear();
                d.Parse(real.c_str(), real.size());
                if (d.HasParseError() || !d.IsObject()) {
            #if defined(DEBUG)
                    XERROR("parse json error");
            #endif

                    return;
                }

                if ((it =d.FindMember("current")) == d.MemberEnd()|| !it->value.IsFloat()) {
            #if defined(DEBUG)
                    XERROR("field doesn't exist");
            #endif

                    return;
                }
                //int high = it->value.GetInt();
                float current = it->value.GetFloat();

                if ((it =d.FindMember("open")) == d.MemberEnd() || !it->value.IsFloat()) {
            #if defined(DEBUG)
                    XERROR("field doesn't exist");
            #endif

                    return;
                }
    //            int open = it->value.GetInt();
                float open = it->value.GetFloat();
                changed = current - open;
            }

            asprintf(&tmp, "{\"BaseCoin\":\"%d\",\"Symbol\":\"%s\",\"SymbolID\":%d,\"UPDown\":%f},",basecoinid,
                     symbolpair, symbolid, changed);

            realmarkans.append(tmp);
            FREE(&tmp);
            FREE(&symbolpair);
        }

//        if (cnt == 0) {
//            //realmarkans[realmarkans.size()] = ']';
//            realmarkans.insert(realmarkans.size(), 1,']');
//        } else {
////            realmarkans[realmarkans.size()-1] = ']';
//            realmarkans.replace(realmarkans.size()-1, 1,"]");
//        }
        realmarkans.replace(realmarkans.size()-1, 1,"]");
        realmarkans.append("}");

        char *routingkey = nullptr;
        asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
        g_tnode->PublishToMQ(routingkey, const_cast<char*>(realmarkans.c_str()), realmarkans.size()+1);
#if defined(DEBUG)
       XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, realmarkans.c_str(), routingkey);
#endif
        FREE(&routingkey);

        //g_redisPool->ReleaseObject(redis);
    }
}

void Consumer::SubscribeHistoryMarket(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parse json error");
#endif
        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if ((it = d.FindMember("Symbol")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit");
#endif
        return;
    }

    const char* symbol = it->value.GetString();

    if ((it = d.FindMember("Timeframe")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit");
#endif
        return;
    }
    const char* timeframe = it->value.GetString();

//    if ((it = d.FindMember("UserID")) == d.MemberEnd()) {
//#if defined(DEBUG)
//        XERROR("field doesn't exsit");
//#endif

//        return ;
//    }
//    int userid = it->value.GetInt();

    if ((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit");
#endif

        return ;
    }
    int wsid = it->value.GetInt();

    if ((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit");
#endif

        return ;
    }
    const char* sessionid = it->value.GetString();

    if ((it = d.FindMember("RequestID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit");
#endif

        return ;
    }
    const char* requestid = it->value.GetString();

    char *tmp = nullptr;
//    asprintf(&tmp, "{\"Tag\":%d,\"UserID\":%d,\"RequestID\":\"%s\",\"SessionID\":\"%s\", "
//                   "\"WSID\":%d,\"MarketHistory\":[",EHISTORYMARKETANS, userid, requestid, sessionid, wsid);
    asprintf(&tmp, "{\"Tag\":%d,\"SessionID\":\"%s\",\"Timeframe\":\"%s\",\"MarketHistory\":[",EHISTORYMARKETANS, sessionid, timeframe);
    std::string historyMark(tmp);
    FREE(&tmp);

    asprintf(&tmp, "%s_%s", symbol, timeframe);

    CRedis *redis= g_redisPool->GetObject();

    if (!redis->SelectDB(MARKET_DB_INDEX)) {
#if defined(DEBUG)
        XERROR("select db error");
#endif
        g_redisPool->ReleaseObject(redis);

        return;
    }


    int ret = redis->ExistsKey(tmp);
    if (!ret || ret == REDIS_REPLY_ERROR) {
#if defined(DEBUG)
        XERROR("key <%s> doesn't exsit, please check this db redis", tmp);
#endif
        FREE(&tmp);
//        redis->SelectDB(0);
//        g_redisPool->ReleaseObject(redis);

//        return;
    }


    int len = HISTORY_MARKET_PAKET_LEN;
    std::shared_ptr<redisReply> ry = redis->Excute("LLEN %s", tmp);
    if (ry.get() != nullptr && ry->type == REDIS_REPLY_INTEGER && ry->integer !=0) {
//        if (ry->integer < HISTORY_MARKET_PAKET_LEN) {
//            len = ry->integer;
//        }
        len = ry->integer;
        ry.reset();
        ry = redis->Excute("LRANGE %s %d %d", tmp, 0, len);

        if (ry->type == REDIS_REPLY_ARRAY) {
            for (int i = ry->elements; i > 0; i--) {
                if (ry->element[i-1]->type == REDIS_REPLY_STRING) {
                    historyMark.append(ry->element[i-1]->str,ry->element[i-1]->len);
                    historyMark.append(",");
                }
            }
//            historyMark[historyMark.size()-1] = ']';
            historyMark.replace(historyMark.size()-1, 1, "]");
        }
    } else {
//        historyMark[historyMark.size()] = ']';
          historyMark.insert(historyMark.size(), 1,']');
    }
    historyMark.append("}");

    char *routingkey = nullptr;
    asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
    g_tnode->PublishToMQ(routingkey, const_cast<char*>(historyMark.c_str()), historyMark.size()+1);
#if defined(DEBUG)
   XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, historyMark.c_str(), routingkey);
#endif
    FREE(&routingkey);

    redis->SelectDB(0);
    g_redisPool->ReleaseObject(redis);
}

void Consumer::SubscribeUserSecurity(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("phase json error|msg=<%s>", event);
#endif

        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if ((it = d.FindMember("UserID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif

        return;
    }
    int userid = it->value.GetInt();

    if ((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif

        return;
    }
    int wsid= it->value.GetInt();

    if ((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif

        return;
    }
    const char *sessionid = it->value.GetString();

    if ((it = d.FindMember("RequestID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif

        return;
    }
    const char *requestid = it->value.GetString();

    MySqlDB db;
    MySqlRecordSet res;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sqlbuf = nullptr;
    asprintf(&sqlbuf, SELECT_FROM_TABLE_USER, userid);
    res = db.QuerySql(sqlbuf);

    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("err=<%s>, con=%d", db.GetMySqlErrMsg(), db.IsConnect());
#endif
        db.Close();
    } else {
        db.Close();
    }

    int rows = res.GetRows();
    if (rows == 0) {
#if defined(DEBUG)
        XERROR("query result is empty|sql=%s", sqlbuf);
#endif
        FREE(&sqlbuf);

        return;
    } else {
        FREE(&sqlbuf);
        //LoginPassword, TradePassword, MobilePhoneNum, IsKYC
        if (rows == 1) {
            std::string loginpassword = res.GetFieldByName(0, "LoginPassword");
            std::string tradepassword = res.GetFieldByName(0, "TradePassword");
            std::string mobphonenum = res.GetFieldByName(0, "MobilePhoneNum");
            std::string iskyc = res.GetFieldByName(0, "IsKYC");

            char *strUserSecurity = nullptr;
            asprintf(&strUserSecurity , "{\"Tag\":%d, \"SessionID\":\"%s\", \"RequestID\":\"%s\",\"loginpassword\":%d, \"tradpassword\":%d, \"mobphonenum\":%d,"
                           "\"iskyc\":%d}", EUSERSECURITYANS, sessionid, requestid, !loginpassword.empty(), !tradepassword.empty(),
                     !mobphonenum.empty(), !iskyc.empty());

            char* routingkey = nullptr;
            asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
            g_tnode->PublishToMQ(routingkey, strUserSecurity, strlen(strUserSecurity)+1);
#if defined(DEBUG)
//            XINFO("ReportSvr-->MQ, reply message:<%s> to MQ with routingkey:<%s>\n", jsonRespone.c_str(), routingkey);
           XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, strUserSecurity, routingkey);
#endif
            FREE(&strUserSecurity);
            FREE(&routingkey);
        }
    }
}

void Consumer::SubscribeCoinCheck(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parse json error, err|<%s>, len=%d", event, eventLen);
#endif

        return;
    }

//    rapidjson::Value::ConstMemberIterator it;
//    if ((it=d.FindMember("UserID")) == d.MemberEnd()) {
//#if defined(DEBUG)
//        XERROR("field doesn't exist");
//#endif
//        return;
//    }
    rapidjson::Value::ConstMemberIterator it;
    if ((it=d.FindMember("RequestID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }
    const char* requestid = it->value.GetString();

    if ((it=d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }
    const char* sessionid = it->value.GetString();

    if ((it=d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }
    int wsid = it->value.GetInt();

    MySqlDB db;
    MySqlRecordSet symbolist;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

//    char* sqlbuf;
//    asprintf(&sqlbuf, SELECT_FROM_TABLE_COIN);
    const char* sqlbuf = SELECT_FROM_TABLE_COIN;
    symbolist = db.QuerySql(sqlbuf);

    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("query error, err=<%s>, con=<%d>|sql=<%s>", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
        db.Close();

        return;
    } else {
        db.Close();
    }

    int rows = symbolist.GetRows();

    char* tmp = nullptr;
    asprintf(&tmp, "{\"Tag\":%d,\"RequestID\":\"%s\",\"SessionID\":\"%s\","
                   "\"SymbolList\":[", ECOINCHECKANS, requestid, sessionid);
    std::string symbolchk(tmp);
    FREE(&tmp);

    if (rows == 0) {
#if defined(DEBUG)
        XERROR("sql query empty|sql=<%>", sqlbuf);
#endif
        //FREE(&sqlbuf);
        return;
    } else {
        //FREE(&sqlbuf);
        for (int i = 0; i < rows; i++) {
            int symbolid = atoi(symbolist.GetFieldByName(i, "ID").c_str());
            std::string coniname = symbolist.GetFieldByName(i, "CoinName");
            std::string describe = symbolist.GetFieldByName(i, "Describe");
            asprintf(&tmp, "{\"ID\":%d,\"CoinName\":\"%s\",\"Describe\":\"%s\"},",
                     symbolid, coniname.c_str(), describe.c_str());

            symbolchk.append(tmp);
            FREE(&tmp);
        }
//        if (rows < 0) {
//            cout << "row < 0 :"<< rows << endl;
//        }
//        symbolchk[symbolchk.size()-1] = ']';
        symbolchk.replace(symbolchk.size()-1, 1, "]");
        symbolchk.append("}");
    }

    char* routingkey = nullptr;
    asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
    g_tnode->PublishToMQ(routingkey, const_cast<char*>(symbolchk.c_str()), symbolchk.size()+1);
#if defined(DEBUG)
   XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, symbolchk.c_str(), routingkey);
#endif
    FREE(&routingkey);
}

void Consumer::SubscribeSymbolCheck(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parse json error, err|<%s>, len=%d", event, eventLen);
#endif

        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if ((it=d.FindMember("RequestID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }
    const char* requestid = it->value.GetString();

    if ((it=d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }
    const char* sessionid = it->value.GetString();

    if ((it=d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }
    int wsid = it->value.GetInt();

    MySqlDB db;
    MySqlRecordSet symbolist;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    const char* sqlbuf = SELECT_FROM_TABLE_SYSMBOL;
    symbolist = db.QuerySql(sqlbuf);

    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("query error, err=<%s>, con=<%d>|sql=<%s>", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
        db.Close();

        return;
    } else {
        db.Close();
    }

    char* tmp = nullptr;
    asprintf(&tmp, "{\"Tag\":%d,\"RequestID\":\"%s\",\"SessionID\":\"%s\","
                   "\"SymbolList\":[", ESYMBOLCHECKANS, requestid, sessionid);
    std::string symbolchk(tmp);
    FREE(&tmp);

    int rows = symbolist.GetRows();
    if (rows == 0) {
#if defined(DEBUG)
        XERROR("sql query empty|sql=<%>", sqlbuf);
#endif

        return;
    } else {
        for (int i = 0; i < rows; i++) {
            int symbolid = atoi(symbolist.GetFieldByName(i, "ID").c_str());
            int basecoinid = atoi(symbolist.GetFieldByName(i,"BaseCoinID").c_str());
            int tradecoinid = atoi(symbolist.GetFieldByName(i, "TradeCoinID").c_str());

            char *symbolpair = nullptr;
            asprintf(&symbolpair, "{\"SymbolID\":%d, \"Symbol\":\"%s/%s\"},", symbolid,
                     g_symbolmap[tradecoinid].c_str(),
                     g_symbolmap[basecoinid].c_str());

           symbolchk.append(symbolpair);
           FREE(&symbolpair);
        }

        symbolchk.replace(symbolchk.size() - 1, 1, "]");
        symbolchk.append("}");

        char* routingkey = nullptr;
        asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
        g_tnode->PublishToMQ(routingkey, const_cast<char*>(symbolchk.c_str()), symbolchk.size()+1);

    #if defined(DEBUG)
       XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, symbolchk.c_str(), routingkey);
    #endif
        FREE(&routingkey);
    }
}

//9
void Consumer::SubscribeLoginHistory(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
        XERROR("parse json error|<%s>", event);
#endif
        return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if ((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }

    int wsid = it->value.GetInt();

    if ((it = d.FindMember("UserID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }

    int userid = it->value.GetInt();

    if ((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }
    const char* sessionid = it->value.GetString();

    if ((it = d.FindMember("RequestID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exist");
#endif
        return;
    }
    const char* requestid = it->value.GetString();

    MySqlRecordSet history_login;
    MySqlDB db;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sqlbuf = nullptr;
    //asprintf(&sqlbuf,"SELECT * from wslogin");

    asprintf(&sqlbuf, SELECT_FROM_TABLE_LOGIN_HISTORY, userid);
    history_login = db.QuerySql(sqlbuf);

    if (db.IsError()) {
#if defined(DEBUG)
        XERROR("%s, con=%d|sql=%s", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
#endif
        db.Close();
    } else {
        db.Close();
    }

    size_t rows = history_login.GetRows();
    if (rows == 0) {
#if defined(DEBUG)
        XWARNING("Empty result|sql=%s", sqlbuf);
#endif
        FREE(&sqlbuf);

        return;
    } else {
        FREE(&sqlbuf);

        char* tmp = nullptr;
        asprintf(&tmp, "{\"Tag\":%d,\"SessionID\":\"%s\",\"RequestID\":\"%s\",\"LoginedList\":[", ELOGINHISTORYANS,
                 sessionid, requestid);
        std::string jsonRespone;
        jsonRespone.append(tmp);
        FREE(&tmp);

        for(int i = 0; i < rows; i++) {
            size_t id = atoi(history_login.GetFieldByName(i, "ID").c_str());
            std::string time = history_login.GetFieldByName(i,"LoginTime");
            std::string ip = history_login.GetFieldByName(i, "LoginIp");
            std::string region = history_login.GetFieldByName(i, "Region");
            std::string action = history_login.GetFieldByName(i, "Action");

            asprintf(&tmp, "{\"ID\":%d,\"LoginTime\":\"%s\",\"LoginIp\":\"%s\",\"Region\":\"%s\",\"Action\":\"%s\"},", id, time.c_str(),
                     ip.c_str(), region.c_str(), action.c_str());
            jsonRespone.append(tmp);
            FREE(&tmp);
        }
//        jsonRespone[jsonRespone.size() - 1] = ']';
        jsonRespone.replace(jsonRespone.size() - 1, 1, "]");
        jsonRespone.append("}");

        char* routingkey = nullptr;
        asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
        g_tnode->PublishToMQ(routingkey, const_cast<char*>(jsonRespone.c_str()), jsonRespone.size()+1);
#if defined(DEBUG)
       XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, jsonRespone.c_str(), routingkey);
#endif
        FREE(&routingkey);
    }
}
//10
void Consumer::SubscribeBalance(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("TaskFunction|<%s...>, deal with message=<%s>, len=%d\n", __func__, event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
#if defined(DEBUG)
       XERROR("parse json error|msg=%s", event);
#endif
       return;
    }

    rapidjson::Value::ConstMemberIterator it;
    if ((it = d.FindMember("UserID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
       XERROR("field does't not exsit!");
#endif

       return;
    }
    int userid = it->value.GetInt();

    if ((it = d.FindMember("WSID")) == d.MemberEnd() || !it->value.IsInt()) {
#if defined(DEBUG)
       XERROR("field does't not exsit!");
#endif

       return;
    }
    int wsid = it->value.GetInt();

//    if((it = d.FindMember("Symbol")) == d.MemberEnd() || !it->value.IsString()) {
//#if defined(DEBUG)
//        XERROR("field doesn't exsit");
//#endif

//        return;
//    }
//    const char* symbol = it->value.GetString();

    if((it = d.FindMember("BaseCoin")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit");
#endif

        return;
    }
    const char* basecoin = it->value.GetString();

    if ((it = d.FindMember("TradeCoin")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
        XERROR("field doesn't exsit");
#endif

        return;
    }
    const char* tradecoin = it->value.GetString();

    if ((it = d.FindMember("SessionID")) == d.MemberEnd() || !it->value.IsString()) {
#if defined(DEBUG)
       XERROR("field does't not exsit!");
#endif

       return;
    }
    const char* sessionid = it->value.GetString();

    char* tmp = nullptr;
    asprintf(&tmp, "{\"Tag\":%d,\"SessionID\":\"%s\",\"UserID\":%d,"
                   "\"WSID\":%d,\"CoinBalance\":[", EBALANCEANS, sessionid, userid, wsid);
    std::string strbalanceans(tmp);
    FREE(&tmp);
    tmp = nullptr;

    MySqlRecordSet available_rep;
    MySqlDB db;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sql = nullptr;

    //asprintf(&sql, SELECT_FROM_TABLE_USER_ASSETS_AND_COIN, userid);
    asprintf(&sql, SELECT_USER_BALANCE, userid, basecoin, tradecoin);
    available_rep =  db.QuerySql(sql);
    if (db.IsError()) {
#if defined(DEBUG)
//        XERROR("%s, err|<%s>", sql, db.GetMySqlErrMsg());
        XERROR("%s,con=%d|sql=%s", db.GetMySqlErrMsg(), db.IsConnect(), sql);
#endif
        db.Close();
    } else {
        db.Close();
    }

    int rows = available_rep.GetRows();
    if (rows == 0) {
#if defined(DEBUG)
        XWARNING("Empty query sql=<%s>", sql);
#endif
        FREE(&sql);
        return;
    } else {
        FREE(&sql);
        float available = 0.0;
        for (int i = 0; i < rows; i++) {
            std::string coinname= available_rep.GetFieldByName(i, "coinname");
            std::cout << coinname <<endl;
            if (strcmp(basecoin, coinname.c_str()) == 0) {
                available = atof(available_rep.GetFieldByName(i, "available").c_str());
                asprintf(&tmp, "{\"BaseCoin\":\"%s\",\"BaseCoinAvailable\":%f},", coinname.c_str(), available);

                strbalanceans.append(tmp);
                FREE(&tmp);
            }

            if (strcmp(tradecoin, coinname.c_str()) == 0) {
                available = atof(available_rep.GetFieldByName(i, "available").c_str());
                asprintf(&tmp, "{\"TradeCoin\":\"%s\",\"TradeCoinAvailable\":%f},", coinname.c_str(), available);

                strbalanceans.append(tmp);
                FREE(&tmp);
            }
        }
//        strbalanceans[strbalanceans.size()-1] = ']';
        strbalanceans.replace(strbalanceans.size()-1, 1,"]");
        strbalanceans.append("}");

        char* routingkey = nullptr;
        asprintf(&routingkey, REPORSERVER2WEB_ROUTINGKEY_ANS, wsid);
        g_tnode->PublishToMQ(routingkey, const_cast<char*>(strbalanceans.c_str()), strbalanceans.size() + 1);
#if defined(DEBUG)
        XINFO("ReportSvr-->MQ|<%s...>, reply message:<%s> to MQ with routingkey:<%s>\n", __func__, strbalanceans.c_str(), routingkey);
#endif
        FREE(&routingkey);
    }
}

void Consumer::SetupMapRequest(void)
{
    m_mapReq.clear();
    m_mapReq.insert(std::make_pair(ETag::EHISTORYORDER , &Consumer::SubscribeHistoryOrderRequest));
    m_mapReq.insert(std::make_pair(ETag::EPENDINGORDER , &Consumer::SubscribePendingOrderRequest));
    m_mapReq.insert(std::make_pair(ETag::EASSETRECORD , &Consumer::SubscribeAssetRequest));
    m_mapReq.insert(std::make_pair(ETag::EBALANCE, &Consumer::SubscribeBalance));
    m_mapReq.insert(std::make_pair(ETag::EMAXMIN, &Consumer::SubscribeMaxMinValue));
    m_mapReq.insert(std::make_pair(ETag::ELOGINHISTORY , &Consumer::SubscribeLoginHistory));
    m_mapReq.insert(std::make_pair(ETag::EMAKETSRV, &Consumer::SubscribeRealMarket));
    m_mapReq.insert(std::make_pair(ETag::EHISTORYMARKET, &Consumer::SubscribeHistoryMarket));
    m_mapReq.insert(std::make_pair(ETag::EUSERSECURITY,&Consumer::SubscribeUserSecurity));
    m_mapReq.insert(std::make_pair(ETag::ECOINCHECK,&Consumer::SubscribeCoinCheck));
    m_mapReq.insert(std::make_pair(ETag::ESYMBOLCHECK,&Consumer::SubscribeSymbolCheck));
    m_mapReq.insert(std::make_pair(ETag::EORDERBOOK,&Consumer::SubscribeOrderBookRequest));
}

Consumer::~Consumer() {
}

std::string Consumer::GetQueueName() {
    return m_queuename;
}

std::string Consumer::GetBindingkey() {
    return m_bindingkey;
}

unsigned int Consumer::ConsumerData(char *pMsg, int nMsgLen) {
#if defined(DEBUG)
    XINFO("MQ-->ReportSvr|<%s...>, recieve message:<%s>, len:<%d> from MQ\n", __func__, pMsg, nMsgLen);
#endif
    int ret = 0;
    regex_t re;
    char errmsg[1023] = {0};

    if ((ret = regcomp(&re, SEARCH_TAG_REGEX, REG_EXTENDED)) != 0) {
        regerror(ret, &re, errmsg, 1023);
        errmsg[1023] = 0;
        fprintf(stderr, "%s", errmsg);
        return 1;
    }
    
    regmatch_t rm[2];
    if ((ret = regexec(&re, pMsg, 2, rm, 0)) != 0) {
        regerror(ret, &re, errmsg, 1023);
        errmsg[1023] = 0;
        fprintf(stderr, "%s", errmsg);
        return 1;
    }
    
    regfree(&re);

    char tagbuf[32] = {0};
    memcpy(tagbuf, &pMsg[rm[1].rm_so], rm[1].rm_eo - rm[1].rm_so);
    int tag = atoi(tagbuf);

#if defined(MULTIPLE_THREAD)
    char* msg = nullptr;
    CMallocSizeNode* cmallocSize = nullptr;
    if (nMsgLen < CMALLOC_SIZE_TNODE)
    {
        cmallocSize = g_cmallocSizePool->GetObject();
        msg = cmallocSize->GetData();
    } else {
        msg = (char*)malloc(nMsgLen);
        if (nullptr == msg) {
            free(msg);
            msg = nullptr;

            //return UNIVERSAL_TNODE_UNKNOWN;
            return 1;
        }
    }

    memcpy(msg, pMsg, nMsgLen);

    g_threadPool->Enqueue([this, msg, nMsgLen, cmallocSize, tag]() {
        std::this_thread::yield();

        MapRequest::iterator it;
        if ((it = m_mapReq.find(tag)) != m_mapReq.end()) {
            (this->*(it->second))(msg, nMsgLen);
        }

        if (nMsgLen < CMALLOC_SIZE_TNODE) {
            g_cmallocSizePool->ReleaseObject(cmallocSize);
        } else {
            if (msg != nullptr) {
                free(msg);
            }
        }
    });
#endif
    //return UNIVERSAL_TNODE_SUCCESS;
    return 1;
}



