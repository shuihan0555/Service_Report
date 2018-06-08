/*************************************************************************
	> File Name: define.h
	> Author: xyz
	> Mail: xiao13149920@foxmail.com 
	> Created Time: Fri 03 Feb 2017 11:33:28 AM CST
 ************************************************************************/
#ifndef SOLUTIONGATEWAY_DEFINE_H_
#define SOLUTIONGATEWAY_DEFINE_H_

#include<chrono>

//#include"bithack.h"
#include"xlogger.h"
#include<chrono>
#include<sys/time.h>
#include<stdexcept>
#include<regex>
#include<string>

class StrLiteral
{
	public:
		template<std::size_t N>
			constexpr StrLiteral(const char(&arr)[N]) noexcept 
			: data{arr}, len{N - 1} {}

		constexpr char operator[](std::size_t i) const
		{
			return i < len ? data[i] : throw std::out_of_range("");
		}

		constexpr std::size_t size() const noexcept { return len; }
		constexpr operator const char*() const noexcept { return data; }
		constexpr const char* c_str() const noexcept { return data; }

	private:
		const char* const data;
		const std::size_t len;
};

#define REQUEST_ID_LEN 33
#define MODEL_NAME_LEN 16
/*SolutionGateway MONITOR*/
//MONITORANS.[SGID] //MonitorAns
constexpr StrLiteral SOLUTIONGATEWAY_PUBLISH_MONITOR_NAME {"MONITORANS"};

constexpr StrLiteral WEB2REPORSERVER_BINDINGKEY_REQUEST {"Web2ReportSrv"};
constexpr StrLiteral WEB2REPORSERVER_QUEUENAME_REQUEST {"ReportSrv_Q1"};
constexpr StrLiteral WEB2REPORTERVER_ROUTINGKEY_REQUEST {"Web2ReportSrv"};

constexpr StrLiteral REPORSERVER2WEB_BINDINGKEY_ANS {"ReportSrv2Web.%d"};
constexpr StrLiteral REPORSERVER2WEB_QUEUENAME_ANS {"WebReportQueue.%d"};
constexpr StrLiteral REPORSERVER2WEB_ROUTINGKEY_ANS {"ReportSrv2Web.%d"};
constexpr StrLiteral SEARCH_TAG_REGEX {"\"Tag\":([0-9]{1,})"};

enum ETag {
	EREQUESTACK = 0x0004,
	/* sendorder */
	ESENDORDER = 0x0011,
	EORDERANS = 0x0012,
	EVALIDATEORDER = 0x0014,
	/* login */
	ELOGIN = 0x0021,
	ELOGINANS = 0x0022, /*34*/
	EUSERINFON = 0x0024,
	EOPOSITION = 0x0026,
	ECPOSITION = 0x0028,
	EOCPOSITION = 0x002A, /*42*/
	/*logout */
	ELOGOUT = 0x0031,
	ELOGOUTANS = 0x0032,
	EUSERINFOT = 0x0034,
	/*deposite*/
	EPULL = 0x0041,
	EPULLANS = 0x0042,
	EOFFLINEORDERANS = 0x0044,
	/*force logout */
	EFORCELOGOUT = 0x0051,
	EFORCELOGOUTANS = 0x0052,

	EMONITOR = 0xA001,
	EMONTORANS = 0xA002,
	/* center logger */
	ECENTERLOG = 0xE001,

//for reportserver    
    EHISTORYORDER = 0x6001,
    EHISTORYORDERANS = 0x6002,
    EPENDINGORDER = 0x6003,//*
    EPENDINGORDERANS = 0x6004,
    EASSETRECORD = 0x6005,
    EASSETRECORDANS = 0x6006,
    EMAKETSRV = 0x6007,
    EMAKETSRVANS = 0x6008,

    EMAXMIN = 0x6011,//*
    EMAXMINANS = 0x6012,
    EBALANCE = 0x6013,
    EBALANCEANS = 0x6014,
//    EPRICERANGE = 0x6015,
//    EPRICERANGEANS = 0x6016,
    ELOGINHISTORY = 0x6017,
    ELOGINHISTORYANS = 0x6018,
    EHISTORYMARKET = 0x6019,
    EHISTORYMARKETANS = 0x6020,
    //
    EUSERSECURITY = 0x6021,
    EUSERSECURITYANS = 0x6022,
    ECOINCHECK = 0x6023,
    ECOINCHECKANS = 0x6024,
    ESYMBOLCHECK = 0x6025,
    ESYMBOLCHECKANS = 0x6026,

    EORDERBOOK = 0x60010,
    EORDERBOOKANS = 0x60011,

    ETESTRECEIVEFROMREPORT = 0x6666,
};

enum ELogin {
	ELOGINDEFAULT = 0x00,
	ELOGINSUCCESS = 0x01,
	ELOGININVALID = 0x02,
	ELOGINUNWRITE = 0x04,
	ELOGINCLOSE = 0x08,
	ELOGINLOGOUT = 0x10,
};


/* from hex to binary */
extern char DecLookupTable[];
void HexToBinary(char* dst, char* src, size_t len);
/* from dec to hex string */
extern char HexLookupTable[513];
extern char hexLookupTable[513];
#define PULL_MSG_SIZE	10
extern const char* AccountErrMsg[PULL_MSG_SIZE];
//void DecToHex(char* dst, char* src, size_t len = 16, bool lower = true);
void DecToHex(char* dst, char* src, size_t len = 10, bool lower = true);


extern const char* SELECT_ORDER_REQUEST_FORMAT;

extern const char* SELECT_HISTORY_ORDERS_ALL;
extern const char* SELECT_HISTORY_ORDERS_BY_SYMBOL;
extern const char* SELECT_HISTORY_ORDERS_BY_SYMBOL_AND_TIME;

extern const char* SELECT_FROM_TABLE_USER_ASSETS;
extern const char* SELECT_FROM_TABLE_COIN;
extern const char* SELECT_FROM_TABLE_SYSMBOL;
extern const char* SELECT_FROM_TABLE_LOGIN_HISTORY;
extern const char* SELECT_FROM_TABLE_USER_ASSETS_AND_COIN;
extern const char* SELECT_USER_BALANCE;
extern const char* SELECT_FROM_TABLE_SYMBOL_WITH_BASECOINNAME;
extern const char* SELECT_FROM_TABLE_USER;
extern const char* POSITION_DB_NAME;

//extern const char* PM_SEND_ORDER_FORMAT;
//extern const char* POSITION_DELETE_PULL_FORMAT;
//extern const char* SG_SEND_ORDER_FORMAT;
//extern const char* SELECT_VALID_FORMAT;
//extern const char* SELECT_VALID_FORMAT_STR;
//extern const char* PM_ORDER_ANS_FORMAT;
//extern const char* SG_ORDER_ANS_FORMAT;
//extern const char* POSITION_UPDATE_VALID_FORMAT;
//extern const char* POSITION_UPDATE_VALID_FORMAT_STR;
//extern const char* POSITION_UPDATE_PULL_FORMAT;
//extern const char* PM_VALIDATE_ORDER_FORMAT;
//extern const char* SG_VALIDATE_ORDER_FORMAT;
//extern const char* POSITION_UPDATE_VALID_WIN_FORMAT;
//extern const char* POSITION_UPDATE_VALID_WIN_FORMAT_STR;
//extern const char* POSITION_UPDATE_VALID_LOSS_FORMAT;
//extern const char* POSITION_UPDATE_VALID_LOSS_FORMAT_STR;
//extern const char* POSITION_UPDATE_VALID_REJECT_FORMAT;
//extern const char* POSITION_UPDATE_VALID_REJECT_FORMAT_STR;
//extern const char* POSITION_UPDATE_VALID_CLOSE_FORMAT;
//extern const char* POSITION_UPDATE_VALID_CLOSE_FORMAT_STR;
//extern const char* INSERT_POSITION_LOG_FORMAT;
//extern const char* INSERT_POSITION_INIT_FORMAT;
//extern const char* SG_CLIENT_LOGIN_FORMAT;
//extern const char* SG_CLIENT_LOGOUT_FORMAT;

template<typename T>
inline void FREE(T** t) {
	if (*t != nullptr) {
		free(*t);
		*t = nullptr;
	}
}

template<typename T>
inline void DELETE(T** t) {
	delete(*t);
	*t = nullptr;
}

inline long GetMicroSecond(void) {
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return t.tv_sec*1000000 + t.tv_nsec/1000;
}

/*OrderQueryAns start*/
struct OrderQueryAns {
public:
    OrderQueryAns();
    ~OrderQueryAns();

    OrderQueryAns(OrderQueryAns& r);
    OrderQueryAns& operator=(OrderQueryAns& r);
    OrderQueryAns(OrderQueryAns&& r);
    OrderQueryAns& operator=(OrderQueryAns&& r);

public:
    const char* ToJson(void);

public:
    std::string Id;
    int UserId;
    std::string Symbol;
    char DelegateType;
    int TransType;
    char ExcelID;
    char OrderIDFrME;
    char Status;
    float Price;
    float Amount;
    float TradeAmount;
    float AveragePrice;
    long orderTime;
    long LastTraderTime;

    char* Json;
};
/*OrderQueryAns end*/

/***MonitorReq start***/
struct  MonitorReq {
public:
	MonitorReq();
	MonitorReq(char* reqID, 
			uint32_t mid, 
			uint64_t timeStamp);
	~MonitorReq();
	MonitorReq(const MonitorReq& r);
	MonitorReq& operator=(const MonitorReq& r);
	MonitorReq(MonitorReq&& r);
	MonitorReq& operator=(MonitorReq&& r);

public:
	const char* ToJson(void);

public:
	char* Json;
	char* ReqID; //[32]; //
	uint32_t Mid;
	uint64_t TimeStamp;//long
};

/***MonitorReq end***/

/***MonitorAns start***/
struct MonitorAns {
public:
	MonitorAns();
	MonitorAns(const char* reqID, 
			char status, 
			const char* name,
			uint32_t pid,
			uint32_t mid,
			uint64_t timeStamp,
			uint64_t startTimes);
	~MonitorAns();
	MonitorAns(const MonitorAns& r);
	MonitorAns& operator=(const MonitorAns& r);
	MonitorAns(MonitorAns&& r);
	MonitorAns& operator=(MonitorAns&& r);

public:
	const char* ToJson(void);

public:	
	char* Json;
	char* ReqID; //[32];
	char  Status;
//	cha	  pad[2] // don't worry about this for now
	char* Name; //[16];
	uint32_t 	Pid;
	uint32_t 	Mid;
	uint64_t	TimeStamp;
	uint64_t	StartTimes;	
};
/***MonitorAns end***/
// status { unknown=0, initiate,  booting,  running,  stopping,  stopped}

void InitDaemon();

#endif 
