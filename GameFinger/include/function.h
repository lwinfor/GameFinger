#ifndef __GM_FUNCTION_H__
#define __GM_FUNCTION_H__

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <list>
#include <vector>

#include "utility.h"

using namespace std;


enum USER_STATUS
{
    E_US_FREE=100,		//����
    E_US_WAITING,		//�ȴ�
    E_US_GAMEINT,		//��Ϸ��
};

struct GameUserInfo
{
    char 			sName[64];
    long 			lScore;
    USER_STATUS  	eStatus;
    int             iSocketFD;     //���˳������
    int             iFightFD;
};

struct GameDetail
{
    int             iUserAFD;     //���˳������
    char            sUserA[10];     //ѡ��Aѡ��
    long            lUserAScore;

    int             iUserBFD;
    char            sUserB[10];     //ѡ��Bѡ��
    long            lUserBScore;

    long            iFightNum;   //��¼���ִ���
};

struct checkUserListRsp
{
    GameUserInfo vUserInfo[USER_MAX];
    int          iUserSize;
};


//�ͻ���������ͨ�����
struct send_info
{ 
    int info_length;            //���͵���Ϣ����ĳ��� 
    char info_content[1024];    //��Ϣ����
    int iFuncIndex;             //
}; 


//���
template<class  T>
void GFdecode(T &t,char *buf)
{
    //�ѽ��յ�����Ϣת���ɽṹ�� 
    memcpy(&t,buf,sizeof(T));
    //cout<<"---decode here---"<<endl;
}

template<class  T>
void GFencode(const T & t,char *buf)
{              
    memcpy(buf,&t,sizeof(t));
    //cout<<"---encode here---"<<endl;
}

int setnonblocking(int sockfd);
void addfd( int epollfd, int fd, bool enable_et );


////client Handle
void clientFuncHand(char * sClientMsg,int & iLastFunc,char *SendMsg);
void clientHandleRsp(char * sClientMsg,int & iLastFunc);


////server Handle
int serverFuncHand(const int &clientfd,list<GameUserInfo> &vClientFD,char *buf,list<GameDetail>  &game_list);
//�û��Ƴ�������������û��б�����Ϸ������Ϣ
void userExit(int clientfd,list<GameUserInfo> &clients_list,list<GameDetail>  &game_list);

void SendToClient(string sMsg,int iFunc,const int &sockfd);

//////////////////////////////////////////////////////////////////////////
void SendUserTotalInfo(const int &clientfd,list<GameUserInfo> &vClientFD);
int sendBroadcastmessage(int clientfd,list<GameUserInfo> &clients_list,list<GameDetail>  &game_list);

#endif