#ifndef REPORTSVR_SUBSCRIBE_H_
#define REPORTSVR_SUBSCRIBE_H_
#include"tnode_adapter.h"
#include<regex.h> /* for regex* */
#include<error.h>

/*SubscribeSendOrder begin */
class SubscribeOrderRequest : public Subscribe {
    public:
        SubscribeOrderRequest(Consumer* consumer = nullptr);
        virtual ~SubscribeOrderRequest() = default;

        SubscribeOrderRequest(const SubscribeOrderRequest& r) =default ;
        SubscribeOrderRequest& operator=(const SubscribeOrderRequest& r) = delete;
        SubscribeOrderRequest(SubscribeOrderRequest&& r) =default ;
        SubscribeOrderRequest& operator=(SubscribeOrderRequest&& r) = delete;

    public:
        virtual void Done(const char* event, unsigned int eventLen) ;
        virtual void Done(char* event, unsigned int eventLen) ;

    public:
        static int StaticInit(void);
    protected:
        static regex_t re;
        /* orderans field + 1: because rm[0] store the all match string*/
        //regmatch_t m_rm[SENDORDER_FIELDS];
};
#endif/*REPORTSVR_SUBSCRIBE_H_ end */

/*SubscribeLoginHistory begin*/
class SubscribeLoginHistory: public Subscribe {
    public:
        SubscribeLoginHistory(Consumer* comsumer = nullptr);
        virtual ~SubscribeLoginHistory() = default;

        SubscribeLoginHistory(SubscribeLoginHistory& loginHistory) = delete;
        SubscribeLoginHistory& operator=(SubscribeLoginHistory& loginHistory) = delete;
        SubscribeLoginHistory(SubscribeLoginHistory &&loginHistory) = delete;
        SubscribeLoginHistory& operator=(SubscribeLoginHistory &&loginHistory) = delete;

    public:
        virtual void Done(const char *event, unsigned int eventLen) {}
        virtual void Done(char *event, unsigned int eventLen);

    private:
        int ID;
        int UserID;
        long LoginTime;
        std::string SrcIp;
};
/*SubscribeLoginHistory end*/
