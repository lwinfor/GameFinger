#include <fstream>
#include "function.h"
#include "time.h"


// 选用list方便删除sockfd
list<GameUserInfo>  clients_list;
//正在游戏的用户列表
list<GameDetail>    game_list;
//记录文件
ofstream fOut;

int   iGameScore=0;
int   iTieScore=0;

void getFileToConf();

//处理逻辑
void GameFighter();
void HandUserLogic(int iRet,const int &sockfd);


void UserGamingHandle(int sockfd);
int computeGameResult(const char *sUserA,const char * sUserB);
void recodeTheGame(const GameDetail & stGameInfo,const int &iRet);



void HandUserLogic(int iRet,const int &sockfd);
int main(int argc, char *argv[])
{

    string sFilePath="GameFighter.txt";
    fOut.open(sFilePath.c_str());


    //读取配置文件，得到每局分数配置
    getFileToConf();

    
    //服务器IP + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    //创建监听socket
    int listener = socket(PF_INET, SOCK_STREAM, 0);
    if(listener < 0) 
    { 
        perror("listener");
        exit(-1);
    }
    cout<<"GameFinger Server Start..."<<endl;
    //绑定地址
    if( bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        cout<<"bind error"<<endl;
        return -1;
    }
    //监听
    int ret = listen(listener, 5);
    if(ret < 0) 
    { 
        cout<<"listen error"<<endl;
        return -1;
    }
    //在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0) 
    { 
        cout<<"epfd error"<<endl;
        return -1;
    }
    //cout<<"epoll created, epollfd = "<<epfd<<endl;
    static struct epoll_event events[EPOLL_SIZE];
    //往内核事件表里添加事件
    addfd(epfd, listener, true);
    //主循环
    while(1)
    {
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0) 
        {
            cout<<"epoll_events_count error"<<endl;
            return -1;
        }
        //处理这epoll_events_count个就绪事件
        for(int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == listener)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept( listener, ( struct sockaddr* )&client_address, &client_addrLength );
                addfd(epfd, clientfd, true);

                {
                    // 服务端用list保存用户连接
                    GameUserInfo stUserInfo;
                    stUserInfo.iSocketFD=clientfd;
                    stUserInfo.eStatus=E_US_FREE;
                    stUserInfo.lScore=0;
                    clients_list.push_back(stUserInfo);
                }

                {
                    //让用户输入名称
                    string sMsg="input your name:\n";
                    SendToClient(sMsg,0,clientfd);
                }
            }
            //处理用户发来的消息，并做逻辑判断
            else
            {
                int iRet = sendBroadcastmessage(sockfd,clients_list,game_list);
                if(iRet < 0) 
                { 
                    cout<<"errro!iRet="<<iRet<<endl;
                }
                else
                {
                    HandUserLogic(iRet,sockfd);
                }
            }
        }
    }
    close(listener); //关闭socket
    close(epfd);    //关闭内核
    fOut.close();
    return 0;
}



void HandUserLogic(int iRet,const int &sockfd)
{
    if (iRet==100)
    {
        //发送菜单
        string sMsg="welcome the GameFinger:\n1.total user list\n2:begin Game\n3:join game\n4:quit\n5:meau";
        SendToClient(sMsg,5,sockfd);
    }
    else if(iRet==2)
    {
        //进行玩家匹配
        //寻找两个处理Waiting状态的用户
        //然后状态更改为游戏中
        UserGamingHandle(sockfd);
    }
    else if(iRet==3)
    {
        if (MORE_LOG)
        {
            cout<<"check The Game"<<endl;
        }
        GameFighter();
    }
    else if(iRet==-100)
    {
        //用户名称不合法，需要重新输入
        string sMsg="input Name unlegel!please input again!";
        SendToClient(sMsg,0,sockfd);
    }
}



void UserGamingHandle(int sockfd)
{

    list<GameUserInfo>::iterator userItem=clients_list.begin();
    bool bHasSuccess=false;
    int iFighterFD=0;
    for (;userItem!=clients_list.end();userItem++)
    {
        if (userItem->iSocketFD==sockfd)
        {
            continue;
        }
        if (userItem->eStatus==E_US_WAITING)
        {
            cout<<"the Fighter is ="<<userItem->iSocketFD<<endl;
            iFighterFD=userItem->iSocketFD;
            bHasSuccess=true;
        }
    }

    if (bHasSuccess)
    {
        list<GameUserInfo>::iterator userItem=clients_list.begin();
        //更改两者的状态，并对两者发送游戏开始
        for (;userItem!=clients_list.end();userItem++)
        {
            if (userItem->iSocketFD==sockfd)
            {
                userItem->iFightFD=iFighterFD;
                userItem->eStatus=E_US_GAMEINT;
            }
            if (userItem->iSocketFD==iFighterFD)
            {
                userItem->iFightFD=sockfd;
                userItem->eStatus=E_US_GAMEINT;
            }
        }
        string sMsg="Game start....\n input:A B C\n";
        SendToClient(sMsg,3,sockfd);
        SendToClient(sMsg,3,iFighterFD);
        {
            //然后加入游戏列表中
            GameDetail stGameItem;
            stGameItem.iUserAFD=sockfd;
            strcpy(stGameItem.sUserA,"\0");
            stGameItem.iUserBFD=iFighterFD;
            strcpy(stGameItem.sUserB,"\0");
            game_list.push_back(stGameItem);
        }
    }
    else
    {
        //匹配战局，让用户等待
        string sMsg="please waiting for gamer...\n";
        SendToClient(sMsg.c_str(),2,sockfd);
    }
}

void GameFighter()
{
    list<GameDetail>::iterator  GameItem=game_list.begin();
    for (;GameItem!=game_list.end();GameItem++)
    {
        if (strcmp(GameItem->sUserA,"\0")!=0&& strcmp(GameItem->sUserB,"\0")!=0)
        {
            //两方都已选择，进行胜负对比
            if (MORE_LOG)
            {
                cout<<"First="<<GameItem->sUserA<<"|Second="<<GameItem->sUserB<<endl;
                cout<<"First="<<GameItem->iUserAFD<<"|Second="<<GameItem->iUserBFD<<endl;
            }
            int iRet=computeGameResult(GameItem->sUserA,GameItem->sUserB);
            string sMsgA="";
            string sMsgB="";
            //然后通知双方
            if(iRet==1)
            {
                sMsgA="You Win!\n";
                sMsgB="You Lose!\n";
                GameItem->lUserAScore+=iGameScore;
                GameItem->lUserBScore-=iGameScore;
            }
            else if(iRet==-1)
            {
                sMsgA="You Lose!\n";
                sMsgB="You Win!\n";
                GameItem->lUserAScore-=iGameScore;
                GameItem->lUserBScore+=iGameScore;
            }
            else
            {
                sMsgA=sMsgB="Tie";
                GameItem->lUserAScore+=iTieScore;
                GameItem->lUserBScore+=iTieScore;
            }
            SendToClient(sMsgA,3,GameItem->iUserAFD);
            SendToClient(sMsgB,3,GameItem->iUserBFD);


            //记录战局
            recodeTheGame(*GameItem,iRet);

            //更改对应状态，记录分数，时间，清除当前选择
            strcpy(GameItem->sUserA,"\0");
            strcpy(GameItem->sUserB,"\0");

            //记录分数
            list<GameUserInfo>::iterator userItem=clients_list.begin();
            for (;userItem!=clients_list.end();userItem++)
            {
                if (userItem->iSocketFD==GameItem->iUserAFD)
                {
                    userItem->lScore+=GameItem->lUserAScore;
                }
                if (userItem->iSocketFD==GameItem->iUserBFD)
                {
                    userItem->lScore+=GameItem->lUserBScore;
                }
            }
        }
    }

}



int computeGameResult(const char *sUserA,const char * sUserB)
{
    cout<<"sUserA="<<sUserA<<"|sUserB="<<sUserB<<endl;
    //A:石头 B:剪刀 C：步
    if (strcmp(sUserA,sUserB)==0)
    {
        return 0;
    }

    if (strcmp(sUserA,"A")==0)
    {
        if (strcmp(sUserB,"B")==0)
        {
            return 1;
        }
        else if(strcmp(sUserB,"C")==0)
        {
            return -1;
        }
    }
    else if (strcmp(sUserA,"B")==0)
    {
        if (strcmp(sUserB,"A")==0)
        {
            return 1;
        }
        else if (strcmp(sUserB,"C")==0)
        {
            return -1;
        }
    }
    else if (strcmp(sUserA,"C")==0)
    {
        if (strcmp(sUserB,"A")==0)
        {
            return 1;
        }
        else if (strcmp(sUserB,"B")==0)
        {
            return -1;
        }
    }
    else
    {
        cout<<"error!-----computeGameResult"<<endl;
    }
    return 0;
}



void recodeTheGame(const GameDetail & stGameInfo,const int &iRet)
{

    //获取对应FD的名称
    string sNameA,sNameB;
    list<GameUserInfo>::iterator userItem=clients_list.begin();
    for (;userItem!=clients_list.end();userItem++)
    {
        if (userItem->iSocketFD==stGameInfo.iUserAFD)
        {
            sNameA=userItem->sName;
        }
        else if (userItem->iSocketFD==stGameInfo.iUserBFD)
        {
            sNameB=userItem->sName;
        }
    }

    //当前时间
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char sTimeString[255] = "\0";
    strftime(sTimeString, sizeof(sTimeString), "%Y-%m-%d %H:%M:%S", timeinfo);


    //比赛结果
    string sResult;
    if (iRet==1)
    {
        sResult=sNameA+" Win";
    }
    else if (iRet==-1)
    {
        sResult=sNameB+" Win";
    }
    else
    {
        sResult="Tie";
    }

    //记录
    fOut<<sTimeString<<"|"<<sNameA<<" VS "<<sNameB<<"|"<<stGameInfo.sUserA<<":"<<stGameInfo.sUserB<<"|"<<sResult<<endl;

    return ;
}

void getFileToConf()
{
    string sFileName="gamefinger.conf";
    ifstream fConf(sFileName.c_str());
    string sLine;
    while (getline(fConf,sLine))
    {
        if (MORE_LOG)
        {
            cout<<"Line="<<sLine<<endl;
        }
        //以等号区分，获取设定参数值
    }
    iGameScore=3;
    iTieScore=1;
}