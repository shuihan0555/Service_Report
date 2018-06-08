#include <stdio.h>
#include<unistd.h>
#include "reportsvr_subscribe.h"
#include"mysqlop.h"
#include<rapidjson/document.h>
#include<rapidjson/writer.h>
#include<rapidjson/rapidjson.h>
#include<rapidjson/stringbuffer.h>

#include"solution_config.h"
#include<gflags/gflags.h>

using namespace snetwork_xservice_db;
using namespace snetwork_xservice_xflagger;
extern TNode *g_tnode;
#define DATA_PACKET_SIZE 15

SubscribeOrderRequest::SubscribeOrderRequest(Consumer *consumer): Subscribe(consumer){
    //std::cout << "do nothing" << endl;
}

void SubscribeOrderRequest::Done(const char* event, unsigned int eventLen) {

}

void SubscribeOrderRequest::Done(char *event, unsigned int eventLen)
{
#if defined(DEBUG)
    XINFO("msg:%s, len:%d\n", event, eventLen);
#endif
    /* Parse Json */
    rapidjson::Document d;
    d.Parse(event, eventLen);
    if (d.HasParseError() || !d.IsObject()) {
        XERROR("parser json error|event=%s", event);

        return;
    }

    rapidjson::Value::ConstMemberIterator it;

//    if(((it = d.FindMember("reqid")) == d.MemberEnd())) {
//        XERROR("error!");

//        return;
//    }
//    char* reqid = "";

    if(((it = d.FindMember("userid")) == d.MemberEnd())) {
        XERROR("error!");

        return;
    }
    int userid = it->value.GetInt();


//    if(((it = d.FindMember("wsid")) == d.MemberEnd())) {
//        XERROR("error!");

//        return;
//    }
//    int wsid = it->value.GetInt();


    std::string orderRequest;
//    orderRequest.append(reqid);

    MySqlRecordSet rs_position;
    MySqlDB db;
    db.Open();
    db.SelectDB(dynamic_cast<SXConfig*>(SXFlagger::GetInstance()->GetConfig())->MySqlDBName().c_str());

    char *sqlbuf = nullptr;
    //asprintf(&sqlbuf, SELECT_ORDER_REQUEST_FORMAT, userid);
    asprintf(&sqlbuf, SELECT_ORDER_REQUEST_ALL_CLOUMNE_FORMAT, userid);

    rs_position = db.QuerySql(sqlbuf);
    if (db.IsError()) {
        XERROR("%s,con=%d|sql=%s", db.GetMySqlErrMsg(), db.IsConnect(), sqlbuf);
        db.Close();
    } else {
        db.Close();
    }

    size_t rows = rs_position.GetRows();
    if (rows == 0) {
        XWARNING("Empty result|sql=%s", sqlbuf);
        FREE(&sqlbuf);

        return;
    } else {
        FREE(&sqlbuf);

        int k = rows/DATA_PACKET_SIZE + 1;
        for(int l = 0; l < k; l++) {
            char* prefix = nullptr;
            asprintf(&prefix, "{\"userid\":%d, \"orderlist\":[", userid);
            std::string jsonRespone;
            jsonRespone.append(prefix);
            FREE(&prefix);

            for(int j = 0; j < DATA_PACKET_SIZE && rows !=0; j++) {
                OrderQueryAns orderqueryans;
                orderqueryans.Id = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "ID").c_str());
                orderqueryans.UserId = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "UserID").c_str());
                orderqueryans.DelegateType = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "DelegateType").c_str());
                orderqueryans.Type = atoi(rs_position.GetFieldByName(DATA_PACKET_SIZE*l+j, "Type").c_str());
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

            jsonRespone[jsonRespone.length() - 1] = ']';
            jsonRespone.append("}");

#if defined(DEBUG)
            XINFO("<orderqueryans>:<%s>, len=%d", jsonRespone.c_str(), jsonRespone.length());
#endif

            g_tnode->PublishToMQ(REPORSERVER_BINDINGKEY_LOGINHISTORY_REQUEST, const_cast<char*>(jsonRespone.c_str()), jsonRespone.length()+1);
        }
    }
}


SubscribeLoginHistory::SubscribeLoginHistory(Consumer *comsumer) : Subscribe(comsumer){

}

void SubscribeLoginHistory::Done(char *event, unsigned int eventLen){
#if defined(DEBUG)
    XINFO("msg=%s, len=%d", event, eventLen);
#endif
    rapidjson::Document d;
    d.Parse(event);
    if (d.HasParseError() || !d.IsObject()) {
        XERROR("parser json error|event=%s", event);

        return;
    }
}
