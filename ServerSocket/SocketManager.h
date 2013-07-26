// SocketManager.h: interface for the CSocketManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOCKETMANAGER_H__7403BD71_338A_4531_BD91_3D7E5B505793__INCLUDED_)
#define AFX_SOCKETMANAGER_H__7403BD71_338A_4531_BD91_3D7E5B505793__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SocketComm.h"

#define WM_UPDATE_CONNECTION	WM_APP+0x1234

#define MAX_SHOW_BUF	512		//界面显示缓冲区
#define MAX_READ_LEN	256		//每次读取文件数据的最大字节数

//用户自定义事件,‘04’之前为以太网连接事件
#define EVT_MSG_SENDALLDATE		0x0004  // response client message "call"

//GG command
#define COMMAND_NUM		4
#define COMMAND_LEN		10

#define AT		1
#define CALL	2
#define SAVA	3
#define CALLALL	4

const char Command[COMMAND_NUM][COMMAND_LEN]=
{
  {"AT"},
  {"CALL"},
  {"SAVE"},
  {"CALLALL"},
} ;

class CSocketManager : public CSocketComm  
{
public:
	CSocketManager();
	virtual ~CSocketManager();

	void SetMessageWindow(CEdit* pMsgCtrl);
	void AppendMessage(LPCTSTR strText );
public:

	virtual void OnDataReceived(const LPBYTE lpBuffer, DWORD dwCount);
	virtual void OnEvent(UINT uEvent, LPVOID lpvData);

protected:
	void DisplayData(const LPBYTE lpData, DWORD dwCount, const SockAddrIn& sfrom);
	CEdit* m_pMsgCtrl;

//tyh:20130711 重构
private:
	bool	m_bIsLoopback;
	bool	m_bIsSave;	//是否将数据报文至文件
	bool	m_bSendAllMsg;	//发送所有历史数据
	char	m_szClntAddress[16];	//client ip
	CFile			m_DataFile;	//用于保存当前连接的收发数据,文件创建规则为"IP1_IP2_IP3_IP4_PORT_DATA"
	CFileException	e;
	CString			m_strPort;
private:
	bool ParseData(char* pData, WORD num, CString strShowMessage);
	int	 GetFileAllData(char* pData, int& num);
	void InitParameters();
	bool SaveToFile(char* pData, char* pDate);
	bool SaveClientIP();
	bool SendAllData();

public:
	BYTE	GetLocalAllAddress(struct in_addr *pAllIp);
	CString BinToString(const char *buffer, size_t len);
	void	StringToBin(const CString& str, char *buffer, size_t len);
	char	CompareCommand(const char* pCommand, BYTE num);

public:
	int		m_nMessageType;		//数据收发的类型


};

#endif // !defined(AFX_SOCKETMANAGER_H__7403BD71_338A_4531_BD91_3D7E5B505793__INCLUDED_)
