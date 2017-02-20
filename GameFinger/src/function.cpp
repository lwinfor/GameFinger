#include "function.h"


int setnonblocking(int sockfd)
{
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)| O_NONBLOCK);
    return 0;
}

void addfd( int epollfd, int fd, bool enable_et )
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;     //输入触发epoll-event
    if( enable_et )
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

/*
@
0:输入名称
1：获取用户列表
2：创建战局
3.加入战局
4.退出
5.菜单
@
*/
void clientFuncHand(char * sClientMsg,int & iLastFunc,char *SendMsg)
{
    send_info stSendInfo;
    //清空缓存 
    memset(stSendInfo.info_content,0,sizeof(stSendInfo.info_content));
    memcpy(stSendInfo.info_content,sClientMsg,sizeof(sClientMsg));

    stSendInfo.info_length=strlen(stSendInfo.info_content);
    stSendInfo.info_content[stSendInfo.info_length-1]='\0';

    if (MORE_LOG)
        cout<<"msg="<<stSendInfo.info_content<<"|length="<<stSendInfo.info_length<<"|iLastFunc="<<iLastFunc<<endl;

    if (iLastFunc!=0)
    {
        //菜单选择
        if (strcmp(stSendInfo.info_content,"1")==0)
        {
            //用户列表
            stSendInfo.iFuncIndex=1;
        }
        else if (strcmp(stSendInfo.info_content,"2")==0)
        {
            //加入战场
            stSendInfo.iFuncIndex=2;            
        }
        else if (iLastFunc==3)
        {
            //进行对战 和玩家进行游戏
            cout<<"playing game...."<<endl;
            stSendInfo.iFuncIndex=3;
        }
        else if (strcmp(stSendInfo.info_content,"4")==0)
        {
            //退出
            stSendInfo.iFuncIndex=4;
        }
        else if (strcmp(stSendInfo.info_content,"5")==0)
        {
            //菜单
            stSendInfo.iFuncIndex=5;
        }
        else
        {
            //cout<<"wrong choose!input 1 2 3 or 4.please input again!"<<endl;
            //return;
            stSendInfo.iFuncIndex=3;
        }
    }
    else
    {
        stSendInfo.iFuncIndex=0;
        iLastFunc=1;
    }
    GFencode(stSendInfo,SendMsg);
    return;
}

void clientHandleRsp(char * sClientMsg,int & iLastFunc)
{
    send_info stRecInfo;
    memset(&stRecInfo,0,sizeof(stRecInfo));
    GFdecode(stRecInfo,sClientMsg);

    if (MORE_LOG)
    {
        cout<<"sizeof(stRecInfo)="<<sizeof(stRecInfo)<<"|sizeof(sClientMsg)="<<strlen(sClientMsg)<<"|Func="<<stRecInfo.iFuncIndex<<endl;
    }

    //判断回包处理
    if (stRecInfo.iFuncIndex==0)
    {
        //初始化，输入名称
        cout<<stRecInfo.info_content<<endl;
        iLastFunc=0;
    }
    else if (stRecInfo.iFuncIndex==1)
    {
        //所有用户列表
        checkUserListRsp stRsp;
        GFdecode(stRsp,stRecInfo.info_content);
        if (MORE_LOG)
        {
            cout<<"SIZE="<<stRsp.iUserSize<<endl;
        }
        for (size_t i=0;i<stRsp.iUserSize;i++)
        {
            cout<<"Name="<<stRsp.vUserInfo[i].sName<<"|Status="<<stRsp.vUserInfo[i].eStatus<<"|Score="<<stRsp.vUserInfo[i].lScore<<endl;
        }
    }
    else if (stRecInfo.iFuncIndex==2)
    {
        //创建战局
        cout<<stRecInfo.info_content<<endl;
    }
    else if (stRecInfo.iFuncIndex==3)
    {
        //加入战局
        cout<<stRecInfo.info_content<<endl;
        iLastFunc=3;
    }
    else if (stRecInfo.iFuncIndex==4)
    {
        //退出
    }
    else if (stRecInfo.iFuncIndex==5)
    {
        //打印菜单
        cout<<stRecInfo.info_content<<endl;
    }
}


int serverFuncHand(const int &clientfd,list<GameUserInfo> &vClientFD,char *buf,list<GameDetail>  &game_list)
{
    send_info stRecInfo;
    memset(&stRecInfo,0,sizeof(stRecInfo));
    GFdecode(stRecInfo,buf);
    if (MORE_LOG)
    {
        cout<<"info_content="<<stRecInfo.info_content<<"|size="<<stRecInfo.info_length<<"|iFunc="<<stRecInfo.iFuncIndex<<endl;
    }
    int iRet=-1;

    if (stRecInfo.iFuncIndex==0)
    {
        //用户注册
        list<GameUserInfo>::iterator ItemIte=vClientFD.begin();
        for (;ItemIte!=vClientFD.end();ItemIte++)
        {
            if (ItemIte->iSocketFD==clientfd)
            {
                //对用户做判断
                if (0)
                {
                    //若不符合要求，重新输入名称（玩家昵称由英文字母、数字组合，首字符不能是数字）
                    return -100;
                }
                //加入用户名称
                memcpy(ItemIte->sName,stRecInfo.info_content,stRecInfo.info_length);
            }
        }
        //发送菜单
        iRet=100;
    }
    else if (stRecInfo.iFuncIndex==1)
    {
        SendUserTotalInfo(clientfd,vClientFD);
        iRet=1;
    }
    else if (stRecInfo.iFuncIndex==2)
    {
        //创建战局
        //将clientfd的状态设置为等待
        list<GameUserInfo>::iterator ItemIte=vClientFD.begin();
        for (;ItemIte!=vClientFD.end();ItemIte++)
        {
            if (ItemIte->iSocketFD==clientfd)
            {
                //加入用户名称
                ItemIte->eStatus=E_US_WAITING;
            }
        }
        iRet=2;
        //然后发送让用户等待
    }
    else if (stRecInfo.iFuncIndex==3)
    {
        //加入战局
        if (MORE_LOG)
        {
            cout<<"clientfd="<<clientfd<<"|use="<<stRecInfo.info_content<<endl;
        }
        string sUserChose=string(stRecInfo.info_content);


        //对用户输入做合法性判断
        if (sUserChose!="A" && sUserChose!="B" && sUserChose!="C")
        {
            string sMsg="Wrong option!input again";
            SendToClient(sMsg,3,clientfd);
            return -99;
        }


        list<GameDetail>::iterator  GameItem=game_list.begin();
        for (;GameItem!=game_list.end();GameItem++)
        {
            if (GameItem->iUserAFD==clientfd)
            {
                strcpy(GameItem->sUserA,stRecInfo.info_content);
            }
            if (GameItem->iUserBFD==clientfd)
            {
                strcpy(GameItem->sUserB,stRecInfo.info_content);
            }
        }
        //需要同时收到两方的信息
        iRet=3;
    }
    else if (stRecInfo.iFuncIndex==4)
    {
        //退出
        close(clientfd);
        userExit(clientfd,vClientFD,game_list);
        iRet=4;
    }
    else if (stRecInfo.iFuncIndex==5)
    {
        //发送菜单
        iRet=100;
    }
    else
    {
        cout<<"error!!!!cann't reg the funtion"<<endl;
    }

    return iRet;
}


void SendUserTotalInfo(const int &clientfd,list<GameUserInfo> &vClientFD )
{
    try
    {
        //处理逻辑数据

        char SendBuf[BUF_SIZE];

        {
            checkUserListRsp stRsp;

            list<GameUserInfo>::const_iterator clientIte=vClientFD.begin();
            int i=0;
            for (;clientIte!=vClientFD.end() && i<USER_MAX;i++,clientIte++)
            {
                stRsp.vUserInfo[i]=*clientIte;
            }
            stRsp.iUserSize=i;

            char Buf[USER_MAX*sizeof(stRsp)];
            GFencode(stRsp,Buf);
            //数据组包
            send_info stRecInfo;
            memset(&stRecInfo,0,sizeof(stRecInfo));
            memcpy(stRecInfo.info_content,Buf,sizeof(Buf));
            stRecInfo.info_length=strlen(stRecInfo.info_content);
            stRecInfo.iFuncIndex=1;
            GFencode(stRecInfo,SendBuf);
            if (MORE_LOG)
            {
                cout<<"stRecInfo.info_length="<<stRecInfo.info_length<<"|SendBuf.size="<<sizeof(SendBuf)<<"|---SendUserTotalInfo---|ClientSize="<<stRsp.iUserSize<<endl;
            }
        }

        if(send(clientfd, SendBuf, BUF_SIZE, 0) < 0 ) 
        { 
            cout<<"error!----"<<endl;
            return;
        }
        //
        //检查所有用户列表
        {
            send_info stRecInfo;
            memset(&stRecInfo,0,sizeof(stRecInfo));
            GFdecode(stRecInfo,SendBuf);

            checkUserListRsp stRsp;
            GFdecode(stRsp,stRecInfo.info_content);
            if (MORE_LOG)
            {
                cout<<"Size="<<stRsp.iUserSize<<endl;
            }
            for (size_t i=0;i<stRsp.iUserSize;i++)
            {
                cout<<"Name="<<stRsp.vUserInfo[i].sName<<"|Status="<<stRsp.vUserInfo[i].eStatus<<"|Score="<<stRsp.vUserInfo[i].lScore<<endl;
            }
        }
    }
    catch(exception &ex)
    {
        cout<<"error!"<<ex.what()<<endl;
        return;
    }
}

void SendToClient(string sMsg,int iFunc,const int &sockfd)
{
    struct send_info stSendInfo;
    memset(stSendInfo.info_content,0,sizeof(stSendInfo.info_content));//清空缓存
    strcpy(stSendInfo.info_content,sMsg.c_str());
    char message[BUF_SIZE];
    bzero(message, BUF_SIZE);
    stSendInfo.info_length=strlen(stSendInfo.info_content);
    stSendInfo.iFuncIndex=iFunc;
    GFencode(stSendInfo,message);
    int ret = send(sockfd, message, BUF_SIZE, 0);
    if(ret < 0) 
    { 
        cout<<"ret="<<ret<<endl;
    }
    if (MORE_LOG)
    {
        cout<<"sMsg="<<sMsg<<"|size0f="<<sizeof(message)<<endl;
    }
    return;
}

int sendBroadcastmessage(int clientfd,list<GameUserInfo> &clients_list,list<GameDetail>  &game_list)
{
    int iRet=-1;
    char buf[BUF_SIZE], message[BUF_SIZE];
    //清零
    bzero(buf, BUF_SIZE);
    bzero(message, BUF_SIZE);

    //cout<<"read from client(clientID = "<<clientfd<<endl;;
    int len = recv(clientfd, buf, BUF_SIZE, 0);
    int iNextFun=0;
    if(len == 0)  // len = 0 client关闭了连接
    {
        close(clientfd);
        userExit(clientfd,clients_list,game_list);
        iRet=0;
    }
    else
    {
        iRet=serverFuncHand(clientfd,clients_list,buf,game_list);
        //cout<<"serverFuncHand iRet="<<iRet<<endl;
    }
    return iRet;
}

void userExit(int clientfd,list<GameUserInfo> &clients_list,list<GameDetail>  &game_list)
{
    //清除游戏对局
    list<GameDetail>::iterator  GameItem=game_list.begin();
    for (;GameItem!=game_list.end();)
    {
        if (GameItem->iUserAFD==clientfd || GameItem->iUserBFD==clientfd)
        {
            //更改游戏状态
            list<GameUserInfo>::iterator clientIte=clients_list.begin();
            for (;clientIte!=clients_list.end();clientIte++)
            {
                if (clientIte->iSocketFD==GameItem->iUserAFD || clientIte->iSocketFD==GameItem->iUserBFD)
                {
                    clientIte->eStatus=E_US_FREE;
                }
            }
            GameItem=game_list.erase(GameItem);
        }
        else
        {
            GameItem++;
        }
    }

    //删除对应在线列表  用户退出
    list<GameUserInfo>::iterator clientIte=clients_list.begin();
    for (;clientIte!=clients_list.end();)
    {
        if (clientIte->iSocketFD==clientfd)
        {
            clientIte=clients_list.erase(clientIte); //list删除fd
            break;
        }
        clientIte++;
    }

}