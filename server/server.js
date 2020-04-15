var fs = require('fs');
const PORT = process.env.PORT || 80;
var dir = './server/uploads';

if (!fs.existsSync(dir)){
    fs.mkdirSync(dir);
}

var https = require('http');
var express = require('express');
const app = express();
app.get('/', (req, res) => {
    res.sendFile(__dirname + '/public/index.html');
})
app.use('/',express.static(__dirname + '/public/'));
var httpsServer = https.createServer(app);
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
	CHANGE_ROOM:	0x06,
	UI_COMMAND:		0x07,
};

var rpc_cb=[], 
	join_rpc = function(id,cb) { rpc_cb[id]=cb; }, 
	process_rpc = function(id,data,client) { if (rpc_cb[id]!=null) rpc_cb[id](data,client); else console.log('unhandled rpc: '+id); }

console.log("[-] mong-speak server [-]");
console.log(" Server listening in port: "+PORT);

app.put('/upload/:filename', function(req, res) {
	var tmpname = Math.random().toString(26).slice(2)+Math.random().toString(26).slice(2);
	var newname = tmpname+'_'+req.params.filename;
	req.pipe(fs.createWriteStream(__dirname + '/uploads/'+newname, {flags: 'w', mode: 0666}));
	res.end(newname)
	setTimeout(()=>{
		fs.unlink(__dirname + './server/uploads/'+newname)
	}, 1000 * 60 * 10) //10 min ttl
});

app.get('/file/:filename',function(req, res) {
	var filepath = './server/uploads/' + req.params.filename
	if (fs.existsSync(filepath)) {
	res.sendFile(filepath)
	} else res.end('')
})

wss.on('connection', (ws) => {
	ws['id']=Math.floor((Math.random() * 9999) + 1111)
	while (wss.clients['filter'] && wss.clients.filter((obj) => obj.id === ws.id).length > 1) {
		ws['id']=Math.floor((Math.random() * 9999) + 1111)
	}
	var roomsbuffer = Buffer.from(JSON.stringify(rooms), "ucs2")
	var rpcbuffer = new Buffer.alloc(3);
	rpcbuffer.writeUInt8(RPCID.ROOMS,0);
	rpcbuffer.writeUInt16LE(null,1);
	var finallbuffer = Buffer.concat([rpcbuffer,roomsbuffer])	
	ws.send(finallbuffer);
	
    ws.on('message', (message) => {
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
	if (d.length==0) {
		ws.close();
		return;
	}
	if (ws.name) return;
	ws['name']=d;
	var pkthead = Buffer.alloc(3);
	pkthead.writeUInt8(RPCID.USER_JOIN,0)
	pkthead.writeUInt16LE(ws.id,1)
	var message = Buffer.concat([pkthead,d])
	ws.send(message)
	wss.clients.forEach((client) => {
		if (client !== ws && client.readyState === 1) {
			
			if (client.name && client.id && client.room_id !== null) {
				var pkthead2 = Buffer.alloc(3);
				pkthead2.writeUInt8(RPCID.USER_JOIN,0)
				pkthead2.writeUInt16LE(client.id,1)
				ws.send(Buffer.concat([pkthead2,client.name]))
				client.send(message)
				
				if (client.room_id != 0) {
					var pktroom = Buffer.alloc(5);
					pktroom.writeUInt8(RPCID.CHANGE_ROOM,0)
					pktroom.writeUInt16LE(client.id,1)
					pktroom.writeUInt16LE(client.room_id,3)
					ws.send(pktroom)
				}
			}
			
		}
	});
})

join_rpc(RPCID.USER_INIT,(d, ws)=>{
	if (d.length!=36) {
		ws.close();
		return;
	}
	ws['guid']=d.toString()
	ws['room_id'] = 0;
})

join_rpc(RPCID.USER_CHAT,(d, ws)=>{
	if (!ws['guid']) {ws.close(); return;}
	if (d.length==0) return;
	var pkthead = Buffer.alloc(3);
	pkthead.writeUInt8(RPCID.USER_CHAT,0)
	pkthead.writeUInt16LE(ws.id,1)
	var message = Buffer.concat([pkthead,d])
	wss.clients.forEach((client) => {
		if (client.readyState === 1 && client.room_id == ws['room_id']) {
			client.send(message);
		}
	});
})

join_rpc(RPCID.UI_COMMAND,(d, ws)=>{
	if (!ws['guid']) {ws.close(); return;}
	if (d.length<2) return;
	var pkthead = Buffer.alloc(3);
	pkthead.writeUInt8(RPCID.UI_COMMAND,0)
	pkthead.writeUInt16LE(ws.id,1)
	var message = Buffer.concat([pkthead,d])
	wss.clients.forEach((client) => {
		if (client.readyState === 1) {
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
		if (client.readyState === 1) {
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
	loadRooms();
}


initialize();