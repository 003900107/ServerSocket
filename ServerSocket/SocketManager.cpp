// SocketManager.cpp: implementation of the CSocketManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <atlconv.h>
#include "ServerSocket.h"
#include "SocketManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/*
const UINT EVT_CONSUCCESS = 0x0000;	// Connection established
const UINT EVT_CONFAILURE = 0x0001;	// General failure - Wait Connection failed
const UINT EVT_CONDROP	  = 0x0002;	// Connection dropped
const UINT EVT_ZEROLENGTH = 0x0003;	// Zero length message
*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSocketManager::CSocketManager()
: m_pMsgCtrl(NULL)
{

}

CSocketManager::~CSocketManager()
{

}


void CSocketManager::DisplayData(const LPBYTE lpData, DWORD dwCount, const SockAddrIn& sfrom)
{
	CString strData;
#ifndef UNICODE
	USES_CONVERSION;
	memcpy(strData.GetBuffer(dwCount), A2CT((LPSTR)lpData), dwCount);
	strData.ReleaseBuffer(dwCount);
#else
	MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<LPCSTR>(lpData), dwCount, strData.GetBuffer(dwCount+1), dwCount+1 );
	strData.ReleaseBuffer(dwCount);
#endif
	if (!sfrom.IsNull())
	{
		LONG  uAddr = sfrom.GetIPAddr();
		BYTE* sAddr = (BYTE*) &uAddr;
		int nPort = ntohs( sfrom.GetPort() ); // show port in host format...
		CString strAddr;
		// Address is stored in network format...
		strAddr.Format(_T("%u.%u.%u.%u (%d)>"),
					(UINT)(sAddr[0]), (UINT)(sAddr[1]),
					(UINT)(sAddr[2]), (UINT)(sAddr[3]), nPort);

		strData = strAddr + strData;
	}

	AppendMessage( strData );
}


void CSocketManager::AppendMessage(LPCTSTR strText )
{
	if (NULL == m_pMsgCtrl)
		return;
/*
	if (::IsWindow( m_pMsgCtrl->GetSafeHwnd() ))
	{
		int nLen = m_pMsgCtrl->GetWindowTextLength();
		m_pMsgCtrl->SetSel(nLen, nLen);
		m_pMsgCtrl->ReplaceSel( strText );
	}
*/
	HWND hWnd = m_pMsgCtrl->GetSafeHwnd();
	DWORD dwResult = 0;
	if (SendMessageTimeout(hWnd, WM_GETTEXTLENGTH, 0, 0, SMTO_NORMAL, 1000L, &dwResult) != 0)
	{
		int nLen = (int) dwResult;
		if (SendMessageTimeout(hWnd, EM_SETSEL, nLen, nLen, SMTO_NORMAL, 1000L, &dwResult) != 0)
		{
			if (SendMessageTimeout(hWnd, EM_REPLACESEL, FALSE, (LPARAM)strText, SMTO_NORMAL, 1000L, &dwResult) != 0)
			{
			}
		}

	}
}


void CSocketManager::SetMessageWindow(CEdit* pMsgCtrl)
{
	m_pMsgCtrl = pMsgCtrl;
}


void CSocketManager::OnDataReceived(const LPBYTE lpBuffer, DWORD dwCount)
{
	LPBYTE lpData = lpBuffer;
	SockAddrIn origAddr;
	stMessageProxy msgProxy;
	CString strShowMessage;

	if (IsSmartAddressing())		//UDP
	{
		dwCount = __min(sizeof(msgProxy), dwCount);
		memcpy(&msgProxy, lpBuffer, dwCount);
		origAddr = msgProxy.address;
		if (IsServer())
		{
			// broadcast message to all
			msgProxy.address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
			WriteComm((const LPBYTE)&msgProxy, dwCount, 0L);
		}
		dwCount -= sizeof(msgProxy.address);
		lpData = msgProxy.byData;
	}
	else		//TCP
	{
		//tyh:20130711 重构，需修改
		if( ParseData((LPSTR)lpBuffer, dwCount, strShowMessage) ) 
		{//返回true，表示收到为数据报文; false,表示为命令报文,不做返送处理

			if(this->IsServer())
			{
				if(m_bIsLoopback==true)
					//res = send( s, pRecvDate, res/*去除换行符*/, 0);
					WriteComm(lpBuffer, dwCount,INFINITE);

				if(m_bSendAllMsg)
				{
					OnEvent( EVT_MSG_SENDALLDATE, NULL );
					m_bSendAllMsg = false;
				}
			}
		}

		// Display data to message list
		DisplayData( /*lpData*/(LPBYTE)(LPCSTR)strShowMessage, 
			strShowMessage.GetLength(), origAddr );
	}
}

///////////////////////////////////////////////////////////////////////////////
// OnEvent
// Send message to parent window to indicate connection status
void CSocketManager::OnEvent(UINT uEvent, LPVOID lpvData)
{
	if (NULL == m_pMsgCtrl)
		return;

	CWnd* pParent = m_pMsgCtrl->GetParent();
	if (!::IsWindow( pParent->GetSafeHwnd()))
		return;

	switch( uEvent )
	{
		//以太网连接事件
		case EVT_CONSUCCESS:
			SaveClientIP();
			AppendMessage( _T("Connection Established\r\n") );
			break;
		case EVT_CONFAILURE:
			AppendMessage( _T("Connection Failed\r\n") );
			break;
		case EVT_CONDROP:
			AppendMessage( _T("Connection Abandonned\r\n") );
			break;
		case EVT_ZEROLENGTH:
			AppendMessage( _T("Zero Length Message\r\n") );
			break;
		//用户自定义事件
		case EVT_MSG_SENDALLDATE:
			SendAllData();
			AppendMessage( _T("Response message'CALLALL', send all data\r\n") );
		default:
			TRACE("Unknown Socket event\n");
			break;
	}

	pParent->PostMessage( WM_UPDATE_CONNECTION, uEvent, (LPARAM) this);

}


///////////////////////////////////////////////////////////////////////////////
// ParseDate
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//      parse receive message
//     
// PARAMETERS:
//		char* pDate:the point to receive date
//		WORD num:Total number of data
// NOTES:
///////////////////////////////////////////////////////////////////////////////
bool  CSocketManager::ParseData(char* pData, WORD num, CString strShowMessage)
{
	char* p;
	CString strData;

	bool bResult = false;
	char cDataTypt;
	char pFileName[32];
	char pDate[24];

	time_t t = time(0);
	p = pData;

	/*
	//获取界面控件
	CWnd* pParent = m_pMsgCtrl->GetParent();
	if (!::IsWindow( pParent->GetSafeHwnd()))
		return false;
	int iSendType = ((CButton *)pParent->GetDlgItem(IDC_ASC))->GetCheck();	//获取IDC_ASC的值
	*/

	//判断数据的收发类型，进行相关处理
	switch(m_nMessageType)
	{
	case 0:
		break;

	case 1:
		strData = BinToString(p, num);
		break;

	default:
		break;
	}


	//组合存储文件名‘pFileName’
	strftime(pDate, sizeof(pDate), "%m%d", localtime(&t));
	sprintf(pFileName, "%.13s_%.7s_%.4s.DAT", m_szClntAddress, m_strPort, pDate);

	//组合当前时间字符串‘pData’
	strftime(pDate, sizeof(pDate), "(%Y-%m-%d %H:%M:%S)", localtime(&t));

	//创建数据记录文件名
	if(this->IsServer())	//服务器
	{
		//组合回显数据
		//sprintf(pShowMessage, "[%.13s]%.21s>>>>>[%d] %.256s\r\n", 
		//	/*szInAddress*/m_szClntAddress, pDate, num, pData);
		//回传用于界面显示的数据长度
		//dwMessageLen = strlen(pShowMessage);

		strShowMessage.Format("[%.13s]%.21s>>>>>[%d] %.256s\r\n", 
			m_szClntAddress, pDate, num, pData);

		/*
		if((*pData=='A') && (*(pData+1)=='T'))//command "AT"
		{
			m_bIsLoopback = !m_bIsLoopback;
		}
		else if((*pData=='S') && (*(pData+1)=='A') 
			&& (*(pData+2)=='V') && (*(pData+3)=='E'))//command "SAVE"
		{
			//修改存储标志	
			m_bIsSave = !m_bIsSave;

			//如果关闭数据写入,需同时关闭数据文件
			if(!m_bIsSave)
			{
				if(m_DataFile.m_hFile != CFile::hFileNull)
				{
					m_DataFile.Close();
				}
			}
			else
			{
				//为了确保打开正确的文件, 如果文件已打开先关闭
				if(m_DataFile.m_hFile != CFile::hFileNull)
				{
					m_DataFile.Close();
				}

				if( !m_DataFile.Open( pFileName, CFile::modeCreate | CFile::modeNoTruncate
					| CFile::modeReadWrite | CFile::shareDenyWrite, &e ) )
				{
					TRACE("File could not be writed!\r\n ");
				}
			}
		}
		else if((*pData=='C') && (*(pData+1)=='A') 
			&& (*(pData+2)=='L') && (*(pData+3)=='L')
			&& (*(pData+4)=='A') && (*(pData+5)=='L') && (*(pData+6)=='L'))//command "CALLALL"
		{
			if(m_DataFile.m_hFile == CFile::hFileNull)
			{
				if( !m_DataFile.Open( pFileName, CFile::modeCreate | CFile::modeNoTruncate
					| CFile::modeReadWrite | CFile::shareDenyWrite, &e ) )
				{
					TRACE("File could not be opened!\r\n ");
				}

			}

			m_bSendAllMsg = true;
			wResult = true;
		}
		else
		{
			if(m_bIsSave)
			{
				//m_DataFile.SeekToEnd();	//指向文件末尾
				//sprintf(pDate+num-1, "%s\r\n", pData);	//添加当前时间, '-1'为去除字符串最后的‘\0’	
				//m_DataFile.Write( pDate, num+20); //原长度为21 最后三个字节分别为“0D 0A 00”，实际写入文件时不需写入“00”，所以长度减“1”
				SaveToFile(pData, pDate);
			}

			wResult = true;
		}
		*/
		cDataTypt = CompareCommand(pData, COMMAND_LEN);
		switch(cDataTypt)
		{
		case AT:
			m_bIsLoopback = !m_bIsLoopback;
			break;

		case CALL:
			break;

		case SAVA:
			//修改存储标志	
			m_bIsSave = !m_bIsSave;

			//如果关闭数据写入,需同时关闭数据文件
			if(!m_bIsSave)
			{
				if(m_DataFile.m_hFile != CFile::hFileNull)
				{
					m_DataFile.Close();
				}
			}
			else
			{
				//为了确保打开正确的文件, 如果文件已打开先关闭
				if(m_DataFile.m_hFile != CFile::hFileNull)
				{
					m_DataFile.Close();
				}

				if( !m_DataFile.Open( pFileName, CFile::modeCreate | CFile::modeNoTruncate
					| CFile::modeReadWrite | CFile::shareDenyWrite, &e ) )
				{
					TRACE("File could not be writed!\r\n ");
				}
			}
			break;

		case CALLALL:
			if(m_DataFile.m_hFile == CFile::hFileNull)
			{
				if( !m_DataFile.Open( pFileName, CFile::modeCreate | CFile::modeNoTruncate
					| CFile::modeReadWrite | CFile::shareDenyWrite, &e ) )
				{
					TRACE("File could not be opened!\r\n ");
				}

			}

			m_bSendAllMsg = true;
			break;

		default:
			if(m_bIsSave)
			{
				//m_DataFile.SeekToEnd();	//指向文件末尾
				//sprintf(pDate+num-1, "%s\r\n", pData);	//添加当前时间, '-1'为去除字符串最后的‘\0’	
				//m_DataFile.Write( pDate, num+20); //原长度为21 最后三个字节分别为“0D 0A 00”，实际写入文件时不需写入“00”，所以长度减“1”
				SaveToFile(pData, pDate);
			}

			bResult = true;
			break;
		}

	}
	else		//客户端，需添加数据处理
	{
		bResult = false;

		//组合回显数据
		//sprintf(pShowMessage, "%.21s>>>>>[%d]%.256s\r\n", pDate, num, pData);
		//回传用于界面显示的数据长度
		//dwMessageLen = strlen(pShowMessage);

		strShowMessage.Format("%.21s>>>>>[%d]%.256s\r\n", pDate, num, pData);
	}

	return bResult;
}



///////////////////////////////////////////////////////////////////////////////
// ParseDate
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//      parse receive message
//     
// PARAMETERS:
//		char* pDate:the point to receive date
//		WORD num:Total number of data
// NOTES:
///////////////////////////////////////////////////////////////////////////////
int CSocketManager::GetFileAllData(char* pData, int& num)
{
	//static long CurrentPosition = -1;
	static long FileLen = -1;
	int result = -1;

	if(m_DataFile.m_hFile == CFile::hFileNull)
	{
		TRACE("File could not be opened!\r\n ");
		return result;
	}

	if(FileLen == -1)
	{
		FileLen = m_DataFile.GetLength();
		m_DataFile.SeekToBegin();
	}

	if( FileLen <= MAX_READ_LEN )
	{
		m_DataFile.Read(pData, FileLen);
		*(pData+FileLen) = '\0';

		FileLen = -1;
		result = 0;

		m_DataFile.Close();
	}
	else
	{
		m_DataFile.Read(pData, MAX_READ_LEN);
		//m_DataFile.Seek(MAX_READ_LEN, CFile::current);

		*(pData+MAX_READ_LEN) = '\0';

		FileLen = FileLen-MAX_READ_LEN;
		result = 1;
	}

	num = strlen(pData)+1; //加上结束符 '\0'的长度

	return result;
}


void CSocketManager::InitParameters()
{
	m_bIsSave = false;
	m_bIsLoopback = false;
	m_bSendAllMsg = false;
	memset(m_szClntAddress, 0, sizeof(m_szClntAddress));
	m_strPort.Format("0000");
}



bool CSocketManager::SaveToFile(char* pData, char* pDate)
{
	WORD len = 0;
	char p[BUFFER_SIZE];

	//为了确保打开正确的文件, 如果文件已打开先关闭
	if(m_DataFile.m_hFile == CFile::hFileNull)
	{
		return false;
	}

	memcpy(p, pData, BUFFER_SIZE);
	m_DataFile.SeekToEnd();	//指向文件末尾

	len = strlen(p);
	sprintf(p+len, "[%d]%s\r\n", len, pDate);	//

	len = strlen(p);
	m_DataFile.Write( p, len); //

	return true;
}


bool CSocketManager::SaveClientIP()	//TYH：保存客户端IP
{
	SOCKADDR_IN addr_conn;
	int nSize = sizeof(addr_conn);
	SOCKET sock = (SOCKET) m_hComm;

	getpeername(sock, (SOCKADDR *)&addr_conn, &nSize);
	strcpy( m_szClntAddress, inet_ntoa(addr_conn.sin_addr) );

	return true;
}


bool CSocketManager::SendAllData()
{
	int iGetAll = -1;
	BYTE frames = 0;
	int iReadLen, iTotalLen;
	CString strShowMessage;

	char pReadData[MAX_READ_LEN];

	iTotalLen = iReadLen = 0;

	while((iGetAll = GetFileAllData(pReadData, iReadLen)) >= 0)
	{
		WriteComm((LPBYTE)pReadData, iReadLen, INFINITE);
		frames++;
		iTotalLen += iReadLen;

		Sleep(200);

		if(iGetAll == 0)
			break;
	}

	if(iGetAll < 0)
	{
		strShowMessage.Format("response 'CALLALL' error!\r\n");
	}
	else
	{
		strShowMessage.Format("response 'CALLALL' send frames=%02d  num=%05d\r\n", 
			frames, iTotalLen);
	}

	AppendMessage((LPCTSTR)strShowMessage);

	return true;
}


BYTE CSocketManager::GetLocalAllAddress(struct in_addr *pAllIp)
{
	BYTE num;
	char host_name[255];
	//获取本地主机名称
	if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR) 
	{
		TRACE("Error %d when getting local host name.\n", WSAGetLastError());
		return 1;
	}
	TRACE("Host name is: %s\n", host_name);
	//从主机名数据库中得到对应的“主机”
	struct hostent *phe = gethostbyname(host_name);
	if (phe == 0) 
	{
		TRACE("Yow! Bad host lookup.");
		return 1;
	}
	//循环得出本地机器所有IP地址
	for (num = 0; phe->h_addr_list[num] != 0; num++) 
	{
		memcpy(pAllIp, phe->h_addr_list[num], sizeof(struct in_addr));
		TRACE("Address %d : %s\n" , num, inet_ntoa(*pAllIp));
		pAllIp++;
	}

	return num;
}


//将二进制转成可见ascii码字符
CString CSocketManager::BinToString(const char *buffer, size_t len) 
{
	static CString str;
	static char s[10000];
	int i,L;

	L=len;
	if (L>10000) 
		L=10000;//忽略10000个后面的数据

	s[0]=0;
	for (i=0;i<L;i++) 
	{
		sprintf(s,"%s%02x ",s, (unsigned  char)buffer[i]);
	}

	if(i ==L)
		str.Format("%s", s);;

	return str;
}


//将ascii码字符转成二进制
void CSocketManager::StringToBin(const CString& str, char *buffer, size_t len) 
{ 
	static char s[10000];
	unsigned int i,L;

	strncpy(s, (LPCSTR)str, str.GetLength());
	L=strlen(s)/3; //去除包含的“空格”

	if(L>len) 
		L=len;

	for(i=0;i<L;i++) 
	{
		sscanf(s+i*3, "%02x", &buffer[i]);
	}

	return;
}

//比较GG command
char CSocketManager::CompareCommand(const char* pCommand, BYTE num)
{
	char result = 0;
	
	for(char i=0; i<COMMAND_NUM; i++)
	{
		if(0 == strncmp(pCommand, Command[i], num))
			return i+1;
	}

	return result;
}