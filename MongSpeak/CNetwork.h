#pragma once
extern CAudioSessionMixer* g_mix;
extern HW_PROFILE_INFOA hwProfileInfo;
extern WebWindow* g_webWindow;

using namespace easywsclient;

class CNetwork {
public:
	CNetwork() {
		lHeartBeatTick = GetTickCount();
		bRequestDisconnect = FALSE;
		szUserName = new wchar_t[50];
		DWORD siz = 50;
		GetComputerName(szUserName, &siz);
		hTask = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CNetwork::NetworkThread, this, NULL, NULL);
	}
	~CNetwork() {
		TerminateThread(hTask, 0);
		Disconnect();
		hTask = NULL;
		if (szUserName)
			delete[] szUserName;
	}
	void SetUserName(wchar_t* name) {
		if (name == NULL) return;
		if (wcslen(name) == 0 || wcslen(name) >= 50) return;
		swprintf_s(szUserName, 50, L"%s", name);
	}
	wchar_t* GetUserName() {
		return szUserName;
	}
	char* GetServerURL() {
		return szServerUrl;
	}
	DWORD NetworkLoop() {
		Sleep(1000);
		INT rc; WSADATA wsaData;
		rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (rc) 
			return 1;
		srand(time(0));
		while (hTask) {
			//connect from this thread
			if (gWS == NULL && szServerUrl != NULL) {
				g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -20, 1);
				gWS = WebSocket::from_url(szServerUrl);
				if (gWS == NULL && szServerUrl != NULL) {
					szServerUrl = NULL;
					g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -20, 3);
				}
			}

			if (bRequestDisconnect) {
				bRequestDisconnect = FALSE;
				if (gWS != NULL) {
					if (Status() > WebSocket::CLOSED)
						gWS->close();
					gWS = NULL;
					szServerUrl = NULL;
					mID = NULL;
				}
			}

			if (gWS && Status() == WebSocket::OPEN) {
				if (!bConnected) {
					//todo: newly connected socket
					string ident(&hwProfileInfo.szHwProfileGuid[1]);
					Send(RPCID::USER_IDENT, &ident);
					SetWindowTextA(g_webWindow->hWndWebWindow, string_format("MongSpeak | %s", &szServerUrl[5]).c_str());
					g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -20, 2);
				}
				bConnected = TRUE;
				try {
				//RX
					for (int c = 0; c < 10; c++) {
						gWS->poll(0);
						gWS->dispatchBinary(&DispatchBinary);
					}
				//TX
					while (pVecBuffer.size() > 0) {
						nBytesWritten += pVecBuffer.front().length();
						gWS->sendBinary((const std::string&)pVecBuffer.front());
						pVecBuffer.pop_front();
					}
				}
				catch (...) {}
				//HEARTBEAT
				if (lHeartBeatTick + 1000 < GetTickCount()) {
					lHeartBeatTick = GetTickCount();
					g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -1, nBytesReaded);
					g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -2, nBytesWritten);
					nBytesReaded = 0; nBytesWritten = 0;
				}
			}
			else {
				if (bConnected) {
					//todo: Connection lost
					Disconnect();
					bConnected = FALSE;
					SetWindowTextA(g_webWindow->hWndWebWindow, "MongSpeak");
					g_webWindow->webForm->QueueCallToEvent(RPCID::USER_LEAVE, mID, (short)0);
					g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -20, 3);
				}
				//todo: not connected
			}
			Sleep(1); 
		}
		WSACleanup();
		return S_OK;
	}
	static void DispatchBinary(const vector<uint8_t>& message) {
		if (message.size() < 3) return;
		extern CNetwork* g_network;
		g_network->nBytesReaded += message.size();
		short sessid = 0;
		rpc_read_short((vector<uint8_t>*)&message, &sessid, 1);
		switch (message.at(0)) {
		case RPCID::USER_JOIN: {
			if (g_network->mID == NULL)
				g_network->mID = sessid;
		} break;
		case RPCID::ROOMS: {
			g_network->Send(RPCID::USER_JOIN, (char*)g_network->szUserName, wcslen(g_network->szUserName) * sizeof(wchar_t));
		} break;
		case RPCID::CHANGE_ROOM: {
			SHORT room = 0;
			if (message.size() != 5) return;
			if (g_network->mID == sessid)
				g_mix->ClearSessions();
			rpc_read_short((vector<uint8_t>*)&message, &room, 3);
			g_webWindow->webForm->QueueCallToEvent(RPCID::CHANGE_ROOM, sessid, room);
			return;
		} break;
		case RPCID::UI_COMMAND: {
			if (message.size() == 5) {
				SHORT cmd = 0;
				rpc_read_short((vector<uint8_t>*)&message, &cmd, 3);
				g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, sessid, cmd);
				return;
			}
		} break;
		case RPCID::OPUS_DATA: {
			if (g_mix != NULL) 
				g_mix->GetSession(sessid)->Decode(&message);
			return;
		} break;
		}
		wchar_t* rest = new wchar_t[((message.size() - 3) / 2) + 1];
		memcpy(rest, message.data() + 3, (message.size() - 3));
		rest[(message.size() - 3) / 2] = L'\0'; 
		g_webWindow->webForm->QueueCallToEvent(message.at(0), sessid, rest);
		delete[] rest;
	}
	void ConnectServer(const char* url) {
		szServerUrl = (char*)url;
	}
	void Disconnect() {
		bRequestDisconnect = TRUE;
	}
	void Send(RPCID id, vector<uint8_t>* pv) {
		if (gWS && Status() == WebSocket::OPEN) {
			string rpc(pv->begin(), pv->end());
			rpc.insert(rpc.begin(), (char)id);
			pVecBuffer.push_back(rpc);
		}
	}
	void Send(RPCID id, string* val) {
		if (gWS && Status() == WebSocket::OPEN) {
			val->insert(val->begin(), (char)id);
			pVecBuffer.push_back(*val);
		}
	}
	void Send(RPCID id, char* data, int len) {
		if (gWS && Status() == WebSocket::OPEN) {
			string rpc(&data[0], &data[len]);
			rpc.insert(rpc.begin(), (char)id);
			pVecBuffer.push_back(rpc);
		}
	}
	int Status() {
		if (!gWS || !this || (DWORD)gWS == 0xDDDDDDDD) return WebSocket::CLOSED;
		return gWS->getReadyState();
	}
	static DWORD NetworkThread(void* obj) {
		return ((CNetwork*)obj)->NetworkLoop();
	}
	LONG nBytesReaded, nBytesWritten;
private:
	HANDLE hTask;
	BOOL bConnected;
	WORD mID;
	WORD mRoom;
	WebSocket* gWS;
	deque<string> pVecBuffer;
	LONG lHeartBeatTick;
	BOOL bRequestDisconnect;
	char* szServerUrl;
	wchar_t* szUserName;
};
