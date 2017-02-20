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
    E_US_FREE=100,		//空闲
    E_US_WAITING,		//等待
    E_US_GAMEINT,		//游戏中
};

struct GameUserInfo
{
    char 			sName[64];
    long 			lScore;
    USER_STATUS  	eStatus;
    int             iSocketFD;     //若退出需更新
    int             iFightFD;
};

struct GameDetail
{
    int             iUserAFD;     //若退出需更新
    char            sUserA[10];     //选手A选项
    long            lUserAScore;

    int             iUserBFD;
    char            sUserB[10];     //选手B选项
    long            lUserBScore;

    long            iFightNum;   //记录交手次数
};

struct checkUserListRsp
{
    GameUserInfo vUserInfo[USER_MAX];
    int          iUserSize;
};


//客户端与服务端通信组包
struct send_info
{ 
    int info_length;            //发送的消息主体的长度 
    char info_content[1024];    //消息主体
    int iFuncIndex;             //
}; 


//解包
template<class  T>
void GFdecode(T &t,char *buf)
{
    //把接收到的信息转换成结构体 
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
//用户推出，需更新在线用户列表与游戏房间信息
void userExit(int clientfd,list<GameUserInfo> &clients_list,list<GameDetail>  &game_list);

void SendToClient(string sMsg,int iFunc,const int &sockfd);

//////////////////////////////////////////////////////////////////////////
void SendUserTotalInfo(const int &clientfd,list<GameUserInfo> &vClientFD);
int sendBroadcastmessage(int clientfd,list<GameUserInfo> &clients_list,list<GameDetail>  &game_list);

#endif