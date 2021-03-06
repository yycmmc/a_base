#include "gameNet.h"
#include <lua.h>
#include <lauxlib.h>
#include <iostream>

#define CACHE_SIZE 0x1000

CGameNet::CGameNet()
: mNetSocket(NULL)
, mSendThRunnable(false)
, mRecvThRunnable(false)
, mNetState(ENetState::None)
, mIp("")
, mPort(0)
{
	mSendBuff = new SBuff();
	mRecvBuff = new SBuff();
}

CGameNet::~CGameNet()
{
	if (mSendBuff)
		delete mSendBuff;

	if (mRecvBuff)
		delete mRecvBuff;
}


CGameNet* CGameNet::GetInstance()
{
	static CGameNet* instance = NULL;
	if (instance == NULL)
	{
		instance = new CGameNet();
	}
	return instance;
}

void CGameNet::_start()
{
	std::thread thSend(SenderThread, this);
	std::thread thRecv(ReceiverThread, this);
	thSend.detach();
	thRecv.detach();
}

void CGameNet::_reconnect()
{
	if (mIp.length() > 0 && mPort > 0)
	{

	}
}

void CGameNet::Update(float _dt)
{
	if (mNetState == ENetState::None)
	{
	}
	else if (mNetState == ENetState::Connected)
	{
	}
	else if (mNetState == ENetState::Connected)
	{
		if (mOnConnectDlg)
			mOnConnectDlg(true);

		_changeState(ENetState::Networking);
	}
	else if (mNetState == ENetState::Networking)
	{
		int a = 1;
	}
	else if (mNetState == ENetState::Disconnected)
	{
		if (mOnDisconnectDlg)
			mOnDisconnectDlg(true);

		_changeState(ENetState::Reconnect);
	}
	else if (mNetState == ENetState::Disconnected)
	{

	}
}

void CGameNet::_sendProc()
{
	char tmpBuff[65535];
	uint32 len;
	while (mSendThRunnable)
	{
		if (mSendBuff->HasOpData())
		{
			len = 0;
			mSendMtx.lock(); //read buff data
			mSendBuff->Read(tmpBuff, &len);
			mSendMtx.unlock();

			if (len > 0)
			{
				int ret = mNetSocket->_Send(tmpBuff, len);
				if (!ret)
				{
					NotifyDisconnected();
					continue;
				}
			}
		}
		else
		{
			Sleep(100);
		}
	}
}

void CGameNet::Send(const char* _buff, int _len)
{
	mSendMtx.lock();
	mSendBuff->Recv(_buff, _len);
	mSendMtx.unlock();
}

void CGameNet::_recvProc()
{
	char tmpBuff[65535];
	int len;
	while (mRecvThRunnable)
	{
		bool ret;
		len = 0;

		ret = mNetSocket->_Recv(tmpBuff, &len);

		if (ret == 0)
		{
			NotifyDisconnected();
			continue;
		}
		else
		{
			if (len > 0)
			{
				mRecvMtx.lock();
				mRecvBuff->Recv(tmpBuff, len);
				mRecvMtx.unlock();
			}
			else
			{
				Sleep(100);
			}
		}
	}
}

void CGameNet::Recv(char* _buff, uint32* _len)
{
	if (mRecvBuff->HasOpData())
	{
		mRecvMtx.lock();
		mRecvBuff->Read(_buff, _len);
		mRecvMtx.unlock();
	}
}

bool CGameNet::_connect(std::string _ip, int _port)
{
	mIp = _ip;
	mPort = _port;

	if (mNetSocket == NULL)
	{
		mNetSocket = new CNetSocket();
	}

	bool result = true;
	if (!mNetSocket->Initialize())
		result = false;

	int errCode = 0;

	if (result && mNetSocket->Connect(_ip.c_str(), _port, errCode))
	{
		printf("---@@@ connect to %s:%d success @@@---\n", _ip.c_str(), _port);
		mSendThRunnable = true;
		mRecvThRunnable = true;
		_start(); //链接成功开始工作线程

		//if (mOnConnectDlg)
		//	mOnConnectDlg(true);

		_changeState(ENetState::Connected);

		return true;
	}
	else
	{
		printf("--------- connect fail, errCode:%f\n", errCode);
		//mSendThRunnable = false;
		//mRecvThRunnable = false;
		if (mOnConnectDlg)
			mOnConnectDlg(false);
		return false;
	}
}

bool CGameNet::Connect(std::string _ip, int _port)
{
	SConnectEntity* entity = new SConnectEntity();
	entity->mNet = this;
	entity->mIp = _ip;
	entity->mPort = _port;

	//std::thread thConnect(ConnectThread, entity);
	//thConnect.detach();
	//printf("---- thConnect:0x%x", &thConnect);
	return ConnectThread(entity);
}

void CGameNet::Close()
{
	mSendThRunnable = false;
	mRecvThRunnable = false;

	if (mNetSocket)
	{
		mNetSocket->Close();
		delete mNetSocket;
		mNetSocket = NULL;
	}
}

void CGameNet::NotifyDisconnected()
{
	printf("--- NotifyDisconnected\n");
	mSendThRunnable = false;
	mRecvThRunnable = false;

	_changeState(ENetState::Disconnected);
}

void CGameNet::ReceiverThread(void* _data)
{
	std::cout << "--- ReceiverThread start\n" << std::endl;
	CGameNet* instance = static_cast<CGameNet*>(_data);
	instance->_recvProc();
}

void CGameNet::SenderThread(void* _data)
{
	printf("--- SenderThread start\n");
	CGameNet* instance = static_cast<CGameNet*>(_data);
	instance->_sendProc();
}

bool CGameNet::ConnectThread(void* _data)
{
	bool ret = false;
	SConnectEntity* entity = static_cast<SConnectEntity*>(_data);
	CGameNet* gameNet = entity->mNet;
	ret = gameNet->_connect(entity->mIp, entity->mPort);
	delete entity;
	return ret;
}

extern "C" {
	bool Connect(const char* _ip, int _port)
	{
		return CGameNet::GetInstance()->Connect(_ip, _port);
	}

	void Send(const char* _msg, int _len) {
		CGameNet::GetInstance()->Send(_msg, _len);
	}

	void Recv(char* _msg, unsigned int* _len) {
		CGameNet::GetInstance()->Recv(_msg, _len);
	}
		
	void Close() {
		CGameNet::GetInstance()->Close();
	}
}



