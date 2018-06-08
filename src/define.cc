/*************************************************************************
	> File Name: define.cc
	> Author: xyz
	> Mail: xiao13149920@foxmail.com 
	> Created Time: Thu 09 Feb 2017 12:20:40 PM CST
 ************************************************************************/
#include<string.h>
#include"define.h"
#include<iomanip>
#include<ctime>
#include<chrono>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<uuid/uuid.h>
#include<string>
#include<list>

//namespace snetwork_xservice_solutiongateway {

extern std::string g_pmid;


char DecLookupTable[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,2,3,4,5,6,7,8,9,	/*0123456789*/
	0,0,0,0,0,0,0, /*:;<=>?@*/
	10,11,12,13,14,15, /*ABCDEF*/
	0,0,0,0,0,0,0,0,0,0,0,0,0, /*GHIJKLMNOPQRS*/
	0,0,0,0,0,0,0,0,0,0,0,0,0, /*TUVWXYZ[/]^_`*/
	10,11,12,13,14,15,	/*abcdef*/
};

char HexLookupTable[513] = {
	"000102030405060708090A0B0C0D0E0F"
	"101112131415161718191A1B1C1D1E1F"
	"202122232425262728292A2B2C2D2E2F"
	"303132333435363738393A3B3C3D3E3F"
	"404142434445464748494A4B4C4D4E4F"
	"505152535455565758595A5B5C5D5E5F"
	"606162636465666768696A6B6C6D6E6F"
	"707172737475767778797A7B7C7D7E7F"
	"808182838485868788898A8B8C8D8E8F"
	"909192939495969798999A9B9C9D9E9F"
	"A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
	"B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
	"C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
	"D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
	"E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
	"F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF"
};

char hexLookupTable[513] = {
	"000102030405060708090a0b0c0d0e0f"
	"101112131415161718191a1b1c1d1e1f"
	"202122232425262728292a2b2c2d2e2f"
	"303132333435363738393a3b3c3d3e3f"
	"404142434445464748494a4b4c4d4e4f"
	"505152535455565758595a5b5c5d5e5f"
	"606162636465666768696a6b6c6d6e6f"
	"707172737475767778797a7b7c7d7e7f"
	"808182838485868788898a8b8c8d8e8f"
	"909192939495969798999a9b9c9d9e9f"
	"a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
	"b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
	"c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
	"d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
	"e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
	"f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"
};

const char* AccountErrMsg[PULL_MSG_SIZE] = {
	"default value",
	"success",
	"margin not enough",
	"formula error(prevbalance+changed+profit!=Margin+FreeMargin)",
	"json error or some field lose",
	"margin is invalid(maybe negative)",
	"pull is not exists",
	"insert fail, database exist records",
	"freemargin is invalid(maybe negative)",
	"unknow error"
};

const char* SELECT_ORDER_REQUEST_FORMAT = "SELECT ID,UserID,DelegateType,Type,ExcelID,OrderIDFrME,Status,Price,Amount,"
                                          "TradeAmount,AveragePrice,OrderTime,LastTraderTime from `token_order` where UserID=%d";

const char* SELECT_HISTORY_ORDERS_ALL =  "SELECT * from `token_order` WHERE UserID=%d order by id desc";
const char* SELECT_HISTORY_ORDERS_BY_SYMBOL = "SELECT * from `token_order` WHERE UserID=%d and Symbol='%s'";
const char* SELECT_HISTORY_ORDERS_BY_SYMBOL_AND_TIME = "SELECT * from `token_order` WHERE UserID=%d and Symbol='%s' and (OrderTime >= %ld and OrderTime <= %ld)";

const char* SELECT_FROM_TABLE_USER_ASSETS = "SELECT a.CoinName, b.Total, b.Available, b.Frozen, b.Valuation FROM Coin as a, User_Assets as b  WHERE b.UserID = %d AND b.CoinID = a.ID";
//const char* SELECT_FROM_TABLE_USER_ASSETS = "SELECT * from User_Assets where UserID=1";
const char* SELECT_FROM_TABLE_COIN = "SELECT ID, CoinName, 'Describe' FROM Coin";
const char* SELECT_FROM_TABLE_SYSMBOL = "SELECT ID ,BaseCoinID, TradeCoinID FROM symbol ORDER BY BaseCoinID";
const char* SELECT_FROM_TABLE_LOGIN_HISTORY = "SELECT ID,LoginTime,LoginIp,Region,Action from Login_History where UserID = %d order by LoginTime desc";
const char* SELECT_FROM_TABLE_USER_ASSETS_AND_COIN = "SELECT a.UserID,a.CoinID,b.CoinName,a.Available FROM User_Assets a , Coin b WHERE a.UserID= %u AND a.CoinID = b.ID";
const char* SELECT_USER_BALANCE = "select a.available,b.coinname from user_assets a, coin b where  a.userid = %d  and a.coinid = b.id  and b.coinname IN ('%s','%s' )";
const char* SELECT_FROM_TABLE_SYMBOL_WITH_BASECOINNAME = "SELECT b.ID, b.BaseCoinID, b.TradeCoinID, a.CoinName FROM symbol b ,"
                                                         "(select ID, CoinName from Coin where CoinName='%s') as a where b.`BaseCoinID` = a.ID";
const char* SELECT_FROM_TABLE_USER = "SELECT LoginPassword, TradePassword, MobilePhoneNum, IsKYC from user where ID = %d";
const char* POSITION_DB_NAME = "exchange";
/*win: 0
 * 1. profit=profit+payout
 * 2. margin=margin-amount
 * 3. balance=balance+payout
 * 4. freemargin=freemargin+payout+amount
 * Open+Change=Margin+FreeMargin-profit
 * Balance=Open+change+profit=Margin+FreeMargin
 * */

/*loss: 
 * 1. profit=profit-amount
 * 2. margin=margin-amount
 * 3. balance=balance-amount
 * Open+Change=Margin+FreeMargin-profit
 * Balance=Open+change+profit=Margin+FreeMargin
 * */

/*reject: 
 * 1. margin=margin-amount
 * 3. freemargin=freemargin+amount
 * Open+Change=Margin+FreeMargin-profit
 * Balance=Open+change+profit=Margin+FreeMargin
 * */

/* close price 
 * 1. profit = profit + payout
 * 2. freemargin = freemain + amount + payout
 * 3. margin = margin - amount
 * 4. balance = balance + payout
 * Balance=Open+change+profit=Margin+FreeMargin
 */


void HexToBinary(char* dst, char* src, size_t len) {
	if ((len & 1) || (src == nullptr) || (dst == nullptr)) {
		return;	/*odd len*/
	}

	char* pDst = dst;
	char* pSrc = src;

	for (size_t i=0; i<len; i +=2) {
		*pDst = DecLookupTable[*pSrc++]<<4;
		*pDst |= DecLookupTable[*pSrc++];
		++pDst;
	}
}

void DecToHex(char* dst, char* src, size_t len, bool lower) {
	if (dst==nullptr || src == nullptr || len == 0) {
		return;
	}

	char* pHex = nullptr;
	if (lower) {
		pHex = hexLookupTable;
	} else {
		pHex = HexLookupTable;
	}
	char* pDst = dst;
	char* pSrc = src;
	int pos = 0;

	for (size_t i=0; i<len; ++i) {
		pos = (*(unsigned char*)pSrc << 1); 
		*pDst++ = pHex[pos];
		*pDst = pHex[pos+1];
		++pDst;
		++pSrc;
	}

	if (lower) {
		strcpy(pDst, "123456789abc");
	} else {
		strcpy(pDst, "123456789ABC");
	}
	strfry(pDst);
	dst[REQUEST_ID_LEN-1] = 0;
}

std::list<std::string> splite(const char* str, const char mark) {
    if (str == nullptr) {
        return std::list<std::string>();
    }

    std::list<std::string> l;
    int i = 0;
    int startp = 0;
    while(str[i] != '\0') {
        if (str[i] != mark) {
            startp = i;
        }
        i++;
    }
    return std::move(l);
}

/*OrderQueryAns*/
OrderQueryAns::OrderQueryAns() :
    Id(""),
    UserId(0),
    Symbol(""),
    DelegateType(0),
    TransType(0),
    ExcelID(0),
    OrderIDFrME(0),
    Status(0),
    Price(0),
    Amount(0),
    TradeAmount(0),
    AveragePrice(0),
    orderTime(0),
    LastTraderTime(0),
    Json(nullptr) {

}

OrderQueryAns::~OrderQueryAns() {
    if (Json != nullptr) {
        free(Json);
        Json = nullptr;
    }
}

OrderQueryAns::OrderQueryAns(OrderQueryAns &r) {
    Id = r.Id;
    UserId = r.UserId;
    Symbol = r.Symbol;
    DelegateType = r.DelegateType;
    TransType = r.TransType;
    ExcelID = r.ExcelID;
    OrderIDFrME = r.OrderIDFrME;
    Status = r.Status;
    Price = r.Price;
    Amount = r.Amount;
    TradeAmount = r.TradeAmount;
    AveragePrice = r.AveragePrice;
    orderTime = r.orderTime;
    LastTraderTime = r.LastTraderTime;


    Json = (char*)malloc(strlen(r.Json));
    memcpy(Json, r.Json, strlen(r.Json));
}

OrderQueryAns& OrderQueryAns::operator =(OrderQueryAns& r) {
    if (this == &r) {
        return *this;
    }

    Id = r.Id;
    UserId = r.UserId;
    Symbol = r.Symbol;
    DelegateType = r.DelegateType;
    TransType = r.TransType;
    ExcelID = r.ExcelID;
    OrderIDFrME = r.OrderIDFrME;
    Status = r.Status;
    Price = r.Price;
    Amount = r.Amount;
    TradeAmount = r.TradeAmount;
    AveragePrice = r.AveragePrice;
    orderTime = r.orderTime;
    LastTraderTime = r.LastTraderTime;


    Json = (char*)malloc(strlen(r.Json));
    memcpy(Json, r.Json, strlen(r.Json));

    return *this;
}

OrderQueryAns::OrderQueryAns(OrderQueryAns &&r) {
    Id = std::move(r.Id);
    UserId = r.UserId;
    Symbol = std::move(r.Symbol);
    DelegateType = r.DelegateType;
    TransType = r.TransType;
    ExcelID = r.ExcelID;
    OrderIDFrME = r.OrderIDFrME;
    Status = r.Status;
    Price = r.Price;
    Amount = r.Amount;
    TradeAmount = r.TradeAmount;
    AveragePrice = r.AveragePrice;
    orderTime = r.orderTime;
    LastTraderTime = r.LastTraderTime;


//    r.Id = 0;
    r.UserId = 0;
//    r.Symbol = 0;
    r.DelegateType = 0;
    r.TransType = 0;
    r.ExcelID = 0;
    r.OrderIDFrME = 0;
    r.Status = 0;
    r.Price = 0;
    r.Amount = 0;
    r.TradeAmount = 0;
    r.AveragePrice = 0;
    r.orderTime = 0;
    r.LastTraderTime = 0;

}

OrderQueryAns& OrderQueryAns::operator =(OrderQueryAns&& r) {
    if (this == &r) {
        return *this;
    }

    Id = std::move(r.Id);
    UserId = r.UserId;
    Symbol = std::move(r.Symbol);
    DelegateType = r.DelegateType;
    TransType = r.TransType;
    ExcelID = r.ExcelID;
    OrderIDFrME = r.OrderIDFrME;
    Status = r.Status;
    Price = r.Price;
    Amount = r.Amount;
    TradeAmount = r.TradeAmount;
    AveragePrice = r.AveragePrice;
    orderTime = r.orderTime;
    LastTraderTime = r.LastTraderTime;


//    r.Id = 0;
    r.UserId = 0;
    r.DelegateType = 0;
    r.TransType = 0;
    r.ExcelID = 0;
    r.OrderIDFrME = 0;
    r.Status = 0;
    r.Price = 0;
    r.Amount = 0;
    r.TradeAmount = 0;
    r.AveragePrice = 0;
    r.orderTime = 0;
    r.LastTraderTime = 0;

    return *this;
}


const char* OrderQueryAns::ToJson(void) {
    if (Json == nullptr) {
        asprintf(&Json, "{\"id\":\"%s\",\"userid\":%d,\"Symbol\":\"%s\",\"delegatetype\":%d,\"Transtype\":%d,\"excelid\":%d,\"orderidfrme\":%d,\"status\":%d,\"price\":%f,\"amount\":%f,\"tradeamount\":%f,\"avarageprice\":%f,\"ordertime\":%ld,\"lasttradertime\":%ld}",
                 Id.c_str(),
                 UserId,
                 Symbol.c_str(),
                 DelegateType,
                 TransType,
                 ExcelID,
                 OrderIDFrME,
                 Status,
                 Price,
                 Amount,
                 TradeAmount,
                 AveragePrice,
                 orderTime,
                 LastTraderTime);
    }
    return Json;
}

/*MonitorReq*/
MonitorReq::MonitorReq() :
	Json(nullptr),
	ReqID(nullptr),
	Mid(0),
	TimeStamp(0) {
	ReqID = (char*) malloc(REQUEST_ID_LEN);
	memset(ReqID, 0, REQUEST_ID_LEN);
}

MonitorReq::MonitorReq(char* reqID, 
	uint32_t mid, 
	uint64_t timeStamp) :
	Json(nullptr),
	ReqID(nullptr),
	Mid(mid),
	TimeStamp(timeStamp) {
	ReqID = (char*) malloc(REQUEST_ID_LEN);
	memcpy(ReqID, reqID, REQUEST_ID_LEN);
}

MonitorReq::~MonitorReq() {
	if(Json != nullptr) {
		free(Json);
		Json = nullptr;
	}

	if(ReqID != nullptr) {
		free(ReqID);
		ReqID = nullptr;
	}
}

MonitorReq::MonitorReq(const MonitorReq& r) {
	Mid = r.Mid;
	TimeStamp = r.TimeStamp;

	ReqID = (char*) malloc(REQUEST_ID_LEN);
	memcpy(ReqID, r.ReqID, REQUEST_ID_LEN);

	if (r.Json != nullptr) {
		size_t len = strlen(r.Json);
		Json = (char*) malloc (len+1);
		memcpy(Json, r.Json, len);
		Json[len] = 0;
	} else {
		Json = nullptr;
	}
}

MonitorReq& MonitorReq::operator=(const MonitorReq& r) {
	if(this == &r) {
		return *this;
	}

	if(Json != nullptr) {
		free(Json);
		Json = nullptr;
	}

	// if(ReqID != nullptr) {
	// 	free(ReqID);
	// 	ReqID = nullptr;
	// }

	Mid = r.Mid;
	TimeStamp = r.TimeStamp;

//	ReqID = (char*) malloc(REQUEST_ID_LEN);
	memcpy(ReqID, r.ReqID, REQUEST_ID_LEN);

	if (r.Json != nullptr) {
		size_t len = strlen(r.Json);
		Json = (char*) malloc (len+1);
		memcpy(Json, r.Json, len);
		Json[len] = 0;
	}

	return *this;
}

MonitorReq::MonitorReq(MonitorReq&& r) {
	Json = r.Json;
	ReqID = r.ReqID;
	Mid = r.Mid;
	TimeStamp = r.TimeStamp;

	r.Json = nullptr;
	r.ReqID = nullptr;
	r.Mid = 0;
	r.TimeStamp = 0;
}

MonitorReq& MonitorReq::operator=(MonitorReq&& r) {
	if(this == &r) {
		return *this;
	}

	if(Json != nullptr) {
		free(Json);
		Json = nullptr;
	}

	if(ReqID != nullptr) {
		free(ReqID);
		ReqID = nullptr;
	}

	Json = r.Json;
	ReqID = r.ReqID;
	Mid = r.Mid;
	TimeStamp = r.TimeStamp;

	r.Json = nullptr;
	r.ReqID = nullptr;
	r.Mid = 0;
	r.TimeStamp = 0;

	return *this;
}

const char* MonitorReq::ToJson(void) {
	if (Json == nullptr) {
		asprintf(&Json, "{\"reqid\":\"%s\",\"mid\":%d,\"timeStamp\":%ld}", 
				 ReqID, 
				 Mid,
				 TimeStamp);
	}

	return Json;
}

MonitorAns::MonitorAns() :
	Json(nullptr),
	ReqID(nullptr),
	Status(0),
	Name(nullptr),
	Pid(0),
	Mid(0),
	TimeStamp(0),
	StartTimes(0) {
	ReqID = (char*) malloc(REQUEST_ID_LEN);
	memset(ReqID, 0, REQUEST_ID_LEN);
	Name = (char*) malloc(MODEL_NAME_LEN);
	memset(Name, 0, MODEL_NAME_LEN);

}

MonitorAns::MonitorAns(const char* reqID, 
	char status, 
	const char* name,
	uint32_t pid,
	uint32_t mid,
	uint64_t timeStamp,
	uint64_t startTimes) : 
	Json(nullptr),
	ReqID(nullptr),
	Status(status),
	Name(nullptr),
	Pid(pid),
	Mid(mid),
	TimeStamp(timeStamp),
	StartTimes(startTimes) {
	ReqID = (char*) malloc(REQUEST_ID_LEN * sizeof(char));
	memcpy(ReqID, reqID, REQUEST_ID_LEN);

	Name = (char*) malloc(MODEL_NAME_LEN + 1);
	int len = strlen(name);
	if(len > MODEL_NAME_LEN) {
		len = MODEL_NAME_LEN;
	}
	memcpy(Name, name, len);
	Name[len] = 0;
	//strncpy(Name, name, MODEL_NAME_LEN);
}	

MonitorAns::~MonitorAns() {
	if(Json != nullptr) {
		free(Json);
		Json = nullptr;
	}

	if(ReqID != nullptr) {
		free(ReqID);
		ReqID = nullptr;
	}

	if(Name != nullptr) {
		free(Name);
		Name = nullptr;
	}
}

MonitorAns::MonitorAns(const MonitorAns& r) {
	Status = r.Status;
	Pid = r.Pid;
	Mid = r.Mid;
	TimeStamp = r.TimeStamp;
	StartTimes = r.StartTimes;

	ReqID = (char*) malloc(REQUEST_ID_LEN);
	memcpy(ReqID, r.ReqID, REQUEST_ID_LEN);

	Name = (char*) malloc(MODEL_NAME_LEN);
	memcpy(Name, r.Name, MODEL_NAME_LEN);

	if (r.Json != nullptr) {
		size_t len = strlen(r.Json);
		Json = (char*) malloc (len+1);
		memcpy(Json, r.Json, len);
		Json[len] = 0;
	} else {
		Json = nullptr;
	}
}

MonitorAns& MonitorAns::operator=(const MonitorAns& r) {
	if(this == &r) {
		return *this;
	}

	if(Json != nullptr) {
		free(Json);
		Json = nullptr;
	}

	memcpy(ReqID, r.ReqID, REQUEST_ID_LEN);
	Status = r.Status;
	memcpy(Name, r.Name, REQUEST_ID_LEN);
	Pid = r.Pid;
	Mid = r.Mid;
	TimeStamp = r.TimeStamp;
	StartTimes = r.StartTimes;

	if (r.Json != nullptr) {
		size_t len = strlen(r.Json);
		Json = (char*) malloc (len+1);
		memcpy(Json, r.Json, len);
		Json[len] = 0;
	}

	return *this;
}

MonitorAns::MonitorAns(MonitorAns&& r) {
	Json = r.Json;
	ReqID = r.ReqID;
	Status = r.Status;
	Name = r.Name;
	Pid = r.Pid;
	Mid = r.Mid;
	TimeStamp = r.TimeStamp;
	StartTimes = r.StartTimes;

	r.Json = nullptr;
	r.ReqID = nullptr;
	r.Status = 0;
	r.Name = nullptr;
	r.Pid = 0;
	r.Mid = 0;
	r.TimeStamp = 0;
	r.StartTimes = 0;
}

MonitorAns& MonitorAns::operator=(MonitorAns&& r) {
	if(this == &r) {
		return *this;
	}

	if(Json != nullptr) {
		free(Json);
		Json = nullptr;
	}
	
	if(ReqID != nullptr) {
		free(ReqID);
		ReqID = nullptr;
	}

	if(Name != nullptr) {
		free(Name);
		Name = nullptr;
	}

	Json = r.Json;
	ReqID = r.ReqID;
	Status = r.Status;
	Name = r.Name;
	Pid = r.Pid;
	Mid = r.Mid;
	TimeStamp = r.TimeStamp;
	StartTimes = r.StartTimes;

	r.Json = nullptr;
	r.ReqID = nullptr;
	r.Status = 0;
	r.Name = nullptr;
	r.Pid = 0;
	r.Mid = 0;
	r.TimeStamp = 0;
	r.StartTimes = 0;

	return *this;
}

const char* MonitorAns::ToJson(void) {
	if (Json == nullptr) {
		asprintf(&Json, "{\"reqid\":\"%s\",\"status\":%d,\"name\":\"%s\",\"pid\":%d,\"mid\":%d,\"timestamp\":%ld,\"starttimes\":%ld}", 
				 ReqID, 
				 Status,
				 Name,
				 Pid,
				 Mid,
				 TimeStamp,
				 StartTimes);
	}

	return Json;
}

void InitDaemon() {
	pid_t pid;
	int i = 0;

	if ((pid = fork()) == -1) {
        printf("Fork error !\n");
		exit(1);
	}
	if (pid != 0) {
		exit(0);        
	}

	setsid();           
	if ((pid = fork()) == -1) {
		printf("Fork error !\n");
		exit(-1);
	}
	if (pid != 0) {
		exit(0);        
	}
	chdir("./");      
	umask(0);           
	for (; i < getdtablesize(); ++i) {
		close(i);       
	}
}
//}
