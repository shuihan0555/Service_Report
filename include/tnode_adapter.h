/*************************************************************************
	> File Name: tnode_adapter.h
	> Author: xyz
	> Mail: xiao13149920@foxmail.com 
	> Created Time: Mon 06 Feb 2017 06:04:58 PM CST
 ************************************************************************/
#ifndef SOLUTIONGATEWAY_TNODE_ADAPTER_H_
#define SOLUTIONGATEWAY_TNODE_ADAPTER_H_
#include<atomic>
#include<vector>
#include<mutex>
#include<map>
#include"tnode.h"
#include"define.h"

using namespace snetwork_xservice_tnode;
#define CMALLOC_SIZE_TNODE	500
template<typename T, size_t SIZE>
struct CMallocSize {
	T data[SIZE];

	T* GetData() {
		return data;
	}
};

/*Subscribe   begin ****/
class Consumer;
class Subscribe {
	public:
        Subscribe(Consumer* consumer);
		virtual ~Subscribe() = default;

		//Subscribe(const Subscribe& r) = default;
		Subscribe(const Subscribe& r);
		Subscribe& operator=(const Subscribe& r) = delete;
		Subscribe(Subscribe&& r);
		Subscribe& operator=(Subscribe&& r) = delete;

		/* parse by rapidson */
		virtual void Done(const char* event, unsigned int eventLen) = 0;
		/* parse by regex */
		virtual void Done(char* event, unsigned int eventLen) = 0;

	protected:
        Consumer* m_consumer;
};
/*Subscribe end ****/

/*Consumer begin ****/
class Consumer : public snetwork_xservice_tnode::TNodeConsumer {
	public:
        explicit Consumer(TNode& tnode, const char* bindingkey, const char *queuename){}
        explicit Consumer(TNode& tnode, const std::string& bindingkey, const std::string& queuename){}
        explicit Consumer(const char* bindingkey, const char* queuename);
		virtual ~Consumer();

		Consumer(const Consumer& r) = delete;
		Consumer& operator=(const Consumer& r) = delete;
		Consumer(Consumer&& r) = delete;
        Consumer& operator=(Consumer&& r) = delete;

	public:
        std::string GetQueueName() override;
        std::string GetBindingkey() override;
        unsigned int ConsumerData(char *pMsg,int nMsgLen) override;
		void CreateSubscibe(void);

private:
        void SubscribePendingOrderRequest(char *event, unsigned int eventLen);//done(redis
        void SubscribeMaxMinValue(char* event, unsigned int eventLen);//done redis
        void SubscribeRealMarket(char* event, unsigned int eventLen);//done redis
        void SubscribeHistoryMarket(char* event, unsigned int eventLen);//done redis
        void SubscribeOrderBookRequest(char *event, unsigned int eventLen);//****redis

        void SubscribeHistoryOrderRequest(char *event, unsigned int eventLen);//done(mysql
        void SubscribeAssetRequest(char *event, unsigned int eventLen);//done(mysql
        void SubscribeLoginHistory(char *event, unsigned int eventLen);//done(mysql
        void SubscribeBalance(char *event, unsigned int eventLen);//done(mysql
        void SubscribeUserSecurity(char *event, unsigned int eventLen);//done(mysql
        void SubscribeCoinCheck(char *event, unsigned int eventLen);//done(mysql
        void SubscribeSymbolCheck(char *event, unsigned int eventLen);//done(mysql

        void SetupMapRequest(void);

private:
        std::string	m_bindingkey;
        std::string	m_queuename;
        
        //int m_tag;
        typedef void (Consumer::*MemberFunPointer)(char *event, unsigned int eventLen);
		typedef std::map<int, MemberFunPointer> MapRequest;
        MapRequest m_mapReq;

};
/*Consumer end ****/

#endif // end of SOLUTIONGATEWAY_TNODE_ADAPTER_H_

