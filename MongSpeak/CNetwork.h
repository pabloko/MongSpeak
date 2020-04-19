#pragma once
/*CNetwork provides a thread to read/write from websocket, using easywebsocket,
handle packets of data and internal custom protocol*/
extern CAudioSessionMixer* g_mix;
extern HW_PROFILE_INFOA hwProfileInfo;
extern vector<wstring> g_jsStack;
extern WebWindow* g_webWindow;
using namespace easywsclient;

class CNetwork {
public:
	CNetwork() {
		//Setup the network thread
		lHeartBeatTick = GetTickCount();
		bRequestDisconnect = FALSE;
		//Obtain default PC name
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
		//Sets username
		//Todo: change this while connected, notification
		if (name == NULL) return;
		if (wcslen(name) == 0 || wcslen(name) >= 50) return;
		swprintf_s(szUserName, 50, L"%s", name);
	}
	wchar_t* GetUserName() {
		//User name for UI
		return szUserName;
	}
	char* GetServerURL() {
		//Server URL for uploader
		return szServerUrl;
	}
	//Network loop
	DWORD NetworkLoop() {
		Sleep(1000); //By this point everithing else should be loaded
		//Init windows ws2_32 token on this thread
		INT rc; WSADATA wsaData;
		rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (rc) 
			return 1;
		srand(time(0)); //We want random to be random
		//Actual loop
		while (hTask) {
			//lets connect when we have a server url but no ws
			if (gWS == NULL && szServerUrl != NULL)
				gWS = WebSocket::from_url(szServerUrl);
			//lets disconnect if requested
			if (bRequestDisconnect) {
				bRequestDisconnect = FALSE;
				//dispose ws state and values
				if (gWS != NULL) {
					if (Status() > WebSocket::CLOSED)
						gWS->close();
					gWS = NULL;
					/*if (szServerUrl)
						delete[] szServerUrl;*/
					szServerUrl = NULL;
					bConnected = FALSE;
					mID = NULL;
				}
			}
			//Check if websocket is connected
			if (gWS && Status() == WebSocket::OPEN) {
				if (!bConnected) {
					//Newly connected socket
					string ident(&hwProfileInfo.szHwProfileGuid[1]);
					//Send USER_IDENT with unique ID
					Send(RPCID::USER_IDENT, &ident);
					//Send USER_JOIN with username
					Send(RPCID::USER_JOIN, (char*)szUserName, wcslen(szUserName)*2);
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
						nBytesWritten += pVecBuffer.at(0).length();
						gWS->sendBinary((const std::string&)pVecBuffer.at(0));
						pVecBuffer.erase(pVecBuffer.begin(), pVecBuffer.begin() + 1);
					}
				}
				catch (...) {}
				//HEARTBEAT
				if (lHeartBeatTick + 1000 < GetTickCount()) {
					lHeartBeatTick = GetTickCount();
					//Send byte tx/rx stats to UI
					g_jsStack.push_back(wstring_format(L"onEvent(%d, %d, %d);", RPCID::UI_COMMAND, -1, nBytesReaded));
					g_jsStack.push_back(wstring_format(L"onEvent(%d, %d, %d);", RPCID::UI_COMMAND, -2, nBytesWritten));
					nBytesReaded = 0; nBytesWritten = 0;
				}
			}
			else { //We're not connected
				if (bConnected) {
					//Connection lost, send self USER_LEAVE to UI
					g_jsStack.push_back(wstring_format(L"onEvent(%d, %d, '');", RPCID::USER_LEAVE, mID));
					Disconnect();
				}
				//Not connected
			}
			Sleep(1); //Prevent spin lock
		}
		WSACleanup();
		return S_OK;
	}
	//RX message handler
	static void DispatchBinary(const vector<uint8_t>& message) {
		//We need at least rpcid+SHORT to read a message
		if (message.size() < 3) return;
		extern CNetwork* g_network;
		//Update RX counter
		g_network->nBytesReaded += message.size();
		//Read session id associated with message
		short sessid = 0;
		rpc_read_short((vector<uint8_t>*)&message, &sessid, 1);
		//Handle several types of RPCID
		switch (message.at(0)) {
		case RPCID::USER_JOIN: {
			//We dont have mID, so its ours (first to come)
			if (g_network->mID == NULL)
				g_network->mID = sessid;
		} break;
		/*case RPCID::USER_CHAT: {
			
		} break;*/
		case RPCID::CHANGE_ROOM: {
			SHORT room = 0;
			//check message size
			if (message.size() != 5) return;
			//If we requested CHANGE_ROOM clear the audio sessions
			if (g_network->mID == sessid)
				g_mix->ClearSessions();
			//Read room id value
			rpc_read_short((vector<uint8_t>*)&message, &room, 3);
			//Notify UI
			g_jsStack.push_back(wstring_format(L"onEvent(%d, %d, %d);", RPCID::CHANGE_ROOM, sessid, room));
			return;
		} break;
		case RPCID::UI_COMMAND: {
			if (message.size() == 5) {
				//Generic handler for UI_COMMAND when last data is a short
				SHORT cmd = 0;
				rpc_read_short((vector<uint8_t>*)&message, &cmd, 3);
				g_jsStack.push_back(wstring_format(L"onEvent(%d, %d, %d);", RPCID::UI_COMMAND, sessid, cmd));
				return;
			}
		} break;
		case RPCID::OPUS_DATA: {
			//Queue opus data to CAudioSession decoder, that will eventually get mixed and rendered.
			if (g_mix != NULL) 
				g_mix->GetSession(sessid)->Decode(&message);
			return;
		} break;
		}
		//Generic message handler [rpcid, short, wstring]
		wchar_t* rest = new wchar_t[((message.size() - 3) / 2) + 1];
		memcpy(rest, message.data() + 3, (message.size() - 3));
		rest[(message.size() - 3) / 2] = L'\0'; 
		//Queue data to be safely loaded on UI thread, also not using stringified to prevent XSS
		g_webWindow->webForm->QueueCallToEvent(message.at(0), sessid, rest);
		delete[] rest;
	}
	void ConnectServer(const char* url) {
		//Set server url, thread will take care of connect if this value changes
		szServerUrl = (char*)url;
	}
	void Disconnect() {
		//Queue a disconnect request on thread
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
private:
	HANDLE hTask;
	BOOL bConnected;
	LONG nBytesReaded, nBytesWritten;
	WORD mID;
	WORD mRoom;
	WebSocket* gWS;
	vector<string> pVecBuffer;
	LONG lHeartBeatTick;
	BOOL bRequestDisconnect;
	char* szServerUrl;
	wchar_t* szUserName;
};
