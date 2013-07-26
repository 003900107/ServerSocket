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

#define MAX_SHOW_BUF	512		//������ʾ������
#define MAX_READ_LEN	256		//ÿ�ζ�ȡ�ļ����ݵ�����ֽ���

//�û��Զ����¼�,��04��֮ǰΪ��̫�������¼�
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

//tyh:20130711 �ع�
private:
	bool	m_bIsLoopback;
	bool	m_bIsSave;	//�Ƿ����ݱ������ļ�
	bool	m_bSendAllMsg;	//����������ʷ����
	char	m_szClntAddress[16];	//client ip
	CFile			m_DataFile;	//���ڱ��浱ǰ���ӵ��շ�����,�ļ���������Ϊ"IP1_IP2_IP3_IP4_PORT_DATA"
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
	int		m_nMessageType;		//�����շ�������


};

#endif // !defined(AFX_SOCKETMANAGER_H__7403BD71_338A_4531_BD91_3D7E5B505793__INCLUDED_)
