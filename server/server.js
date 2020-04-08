var fs = require('fs');
const PORT = 8083;

//var privateKey = fs.readFileSync( '../ssl.key' );
//var certificate = fs.readFileSync( '../ssl.fullchain.cer' );

//var privateKey = fs.readFileSync(__dirname + '/cert/key.pem', 'utf8');
//var certificate = fs.readFileSync(__dirname + '/cert/cert.pem', 'utf8');

//var credentials = { key: privateKey, cert: certificate };
var https = require('http');
var express = require('express');
const app = express();
app.get('/', (req, res) => {
    res.sendFile(__dirname + '/public/index.html');
})
app.use('/',express.static(__dirname + '/public/'));
var httpsServer = https.createServer(/*credentials,*/ app);
httpsServer.listen(PORT);
var WebSocketServer = require('ws').Server;
var wss = new WebSocketServer({server: httpsServer});

var roomsObject = require('./rooms.json');

var rooms = [];

var RPCID = {
	USER_INIT: 		0x00,
	USER_JOIN: 		0x01,
	USER_LEAVE: 	0x02,
	USER_CHAT: 		0x03,
	OPUS_DATA: 		0x04,
	ROOMS:			0x05,
	CHANGE_ROOM:	0x06
};

var rpc_cb=[], 
	join_rpc = function(id,cb) { rpc_cb[id]=cb; }, 
	process_rpc = function(id,data,client) { if (rpc_cb[id]!=null) rpc_cb[id](data,client); else console.log('unhandled rpc: '+id); }

console.log("[-] mong-speak server [-]");
console.log(" Server listening in port: "+PORT);

wss.on('connection', (ws) => {
	//console.log("[WebSocketServer]connection")
	ws['id']=Math.floor((Math.random() * 9999) + 1111)
	//console.log(wss.clients)
	while (wss.clients['filter'] && wss.clients.filter((obj) => obj.id === ws.id).length > 1) {
		ws['id']=Math.floor((Math.random() * 9999) + 1111)
	}
	//Update to the connected client about the existing rooms
	var roomsbuffer = Buffer.from(JSON.stringify(rooms), "ucs2")
	var rpcbuffer = new Buffer.alloc(3);
	rpcbuffer.writeUInt8(RPCID.ROOMS,0);
	rpcbuffer.writeUInt16LE(null,1);
	var finallbuffer = Buffer.concat([rpcbuffer,roomsbuffer])	
	ws.send(finallbuffer);
	
    ws.on('message', (message) => {
		//console.log("[WebSocketServer]message", message)
		//console.log('raw',message)
		process_rpc(message[0], message.slice(1), ws)
    });
	
	ws.on('close', () => {
		if (!ws['guid']) {ws.close(); return;}
		if (ws.id) {
			var epkthead = Buffer.alloc(3);
			epkthead.writeUInt8(RPCID.USER_LEAVE,0)
			epkthead.writeUInt16LE(ws.id,1)
			wss.clients.forEach((client, idx) => {
			  	if (client !== ws && client.readyState === 1) {
					client.send(epkthead)
				}
				if(client == ws) clients.splice(idx, 1);
			});
		}
	})	
});

join_rpc(RPCID.USER_JOIN,(d, ws)=>{
	if (!ws['guid']) {ws.close(); return;}
	//console.log("[WebSocketServer::RPC]->USER_JOIN",d)
	if (d.length==0) {
		ws.close();
		return;
	}
	if (ws.name) return;
	var name = d.toString('utf8');
	ws['name']=name;
	var pkthead = Buffer.alloc(3+name.length);
	pkthead.writeUInt8(RPCID.USER_JOIN,0)
	pkthead.writeUInt16LE(ws.id,1)
	pkthead.write(name,3)
	ws.send(pkthead) //stream the new client to own client
	
	wss.clients.forEach((client) => {
		if (client !== ws && client.readyState === 1) {
			
			if (client.name && client.id) {
				var pkthead2 = Buffer.alloc(3+client.name.length);
				pkthead2.writeUInt8(RPCID.USER_JOIN,0)
				pkthead2.writeUInt16LE(client.id,1)
				pkthead2.write(client.name,3)
				ws.send(pkthead2) //stream the remote client to new client
				client.send(pkthead) //stream to the remote client the new client
			}
			
		}
	});
})

join_rpc(RPCID.USER_INIT,(d, ws)=>{
	if (d.length!=36) {
		ws.close();
		return;
	}
	//console.log("[WebSocketServer::RPC]->USER_INIT",d.toString())
	ws['guid']=d.toString()
	ws['room_id'] = 0; //assign default room lobby
})

join_rpc(RPCID.USER_CHAT,(d, ws)=>{
	if (!ws['guid']) {ws.close(); return;}
	if (d.length==0) return;
	var pkthead = Buffer.alloc(3);
	pkthead.writeUInt8(RPCID.USER_CHAT,0)
	pkthead.writeUInt16LE(ws.id,1)
	var message = Buffer.concat([pkthead,d])
	wss.clients.forEach((client) => {
		if (/*client !== ws &&*/ client.readyState === 1) {
			client.send(message);
		}
	});
})


join_rpc(RPCID.OPUS_DATA,(d, ws)=>{
	if (!ws['guid']) {ws.close(); return;}
	if (d.length==0) return;
	var pkthead = Buffer.alloc(3);
	pkthead.writeUInt8(RPCID.OPUS_DATA,0)
	pkthead.writeUInt16LE(ws.id,1)
	var message = Buffer.concat([pkthead,d])
	wss.clients.forEach((client) => {
		if (client !== ws && client.readyState === 1 && client.room_id == ws['room_id']) {
			client.send(message);
		}
	});
})

join_rpc(RPCID.CHANGE_ROOM, (d, ws) => {
	if (!ws['guid']) {ws.close(); return;}
	if (d.length!=2) return;
	var room = d.readUInt16LE(0)
	var pkthead = Buffer.alloc(5);
	pkthead.writeUInt8(RPCID.CHANGE_ROOM,0)
	pkthead.writeUInt16LE(ws.id,1)
	pkthead.writeUInt16LE(room, 3)
	ws['room_id'] = room;
	wss.clients.forEach((client) => {
		if (/*client !== ws &&*/ client.readyState === 1) {
			client.send(pkthead);
		}
	});
});

function loadRooms(){
	roomsObject.forEach((room)=>{
		rooms.push(room);
	});
}

function initialize(){
	//Load rooms
	loadRooms();
}


initialize();