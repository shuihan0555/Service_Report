/*************************************************************************
	> File Name: test_safe_list.cc
	> Author: xyz
	> Mail: xiao13149920@foxmail.com 
	> Created Time: Wed 15 Feb 2017 03:05:18 PM CST
 ************************************************************************/

#include"safe_list.h"
#include<iostream>
using namespace std;
//using namespace snetwork_xservice_solutiongateway;

int main(int argc, char* argv[]) {
	SafeList<int> l;
	for (size_t i=0; i<10; ++i) {
		l.PushFront(i);
	}

	cout<<"Length="<<l.Length()<<endl;
	l.ForEach([](int a){cout<<a<<"	";});
	auto data = l.Find(8);
	cout<<endl<<endl;
	l.Remove([](int a){return a==6;});
	l.ForEach([](int a){cout<<a<<"	";});
	cout<<endl<<endl;
	int da = 7;
	bool flag = l.Update(7, [&](int& a){ std::cout<<a<<endl;
			std::cout<<da<<endl;
			{
				int i;
				if (1==1) {

				}
			}
			a *= da+100;});
	l.ForEach([](int a){cout<<a<<"	";});
	cout<<endl<<flag<<endl;


	return 0;
}
