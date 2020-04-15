var fs = require('fs');
const PORT = process.env.PORT || 80;
var dir = './uploads';

if (!fs.existsSync(dir)){
    fs.mkdirSync(dir);
}
//var privateKey = fs.readFileSync( '../ssl.key' );
//var certificate = fs.readFileSync( '../ssl.fullchain.cer' );

//var privateKey = fs.readFileSync(__dirname + '/cert/key.pem', 'utf8');
//var certificate = fs.readFileSync(__dirname + '/cert/cert.pem', 'utf8');

//var credentials = { key: privateKey, cert: certificate };
var https = require('http');
var express = require('express');
const fileUpload = require('express-fileupload');
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
	CHANGE_ROOM:	0x06,
	UI_COMMAND:		0x07,
};

var rpc_cb=[], 
	join_rpc = function(id,cb) { rpc_cb[id]=cb; }, 
	process_rpc = function(id,data,client) { if (rpc_cb[id]!=null) rpc_cb[id](data,client); else console.log('unhandled rpc: '+id); }

console.log("[-] mong-speak server [-]");
console.log(" Server listening in port: "+PORT);

// default options
app.use(fileUpload({
	limits: {
        fileSize: 10000000, //10mb
		abortOnLimit: true
    }
}));

app.put('/upload/:filename', function(req, res) {
	var tmpname = Math.random().toString(26).slice(2)+Math.random().toString(26).slice(2);
	var newname = tmpname+'_'+req.params.filename;
	req.pipe(fs.createWriteStream(__dirname + '/uploads/'+newname, {flags: 'w', mode: 0666}));
	res.end(newname)
	setTimeout(()=>{
		fs.unlink(__dirname + '/uploads/'+newname)
	}, 1000 * 60 * 10) //10 min ttl
});

app.get('/file/:filename',function(req, res) {
	var filepath = __dirname + '/uploads/' + req.params.filename
	if (fs.existsSync(filepath)) {
	res.sendFile(filepath)
	} else res.end('')
})

/*app.put('/upload', function(req, res) {
	if (!req.files || Object.keys(req.files).length === 0) {
		return res.status(400).send('No files were uploaded.');
	}

	// The name of the input field (i.e. "sampleFile") is used to retrieve the uploaded file
	let sampleFile = req.files;//.sampleFile;
	console.log(sampleFile)

	// Use the mv() method to place the file somewhere on your server
	sampleFile.mv('./uploads/'+req.body.userid+'_filename.jpg', function(err) {
		if (err)
			return res.status(500).send(err);

		res.sendFile(__dirname + '/uploads/'+req.body.userid+'_filename.jpg');

	});
});*/
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
	//var name = d.toString('ucs2');
	ws['name']=d;
	var pkthead = Buffer.alloc(3);
	pkthead.writeUInt8(RPCID.USER_JOIN,0)
	pkthead.writeUInt16LE(ws.id,1)
	var message = Buffer.concat([pkthead,d])
	ws.send(message) //stream the new client to own client
	
	wss.clients.forEach((client) => {
		if (client !== ws && client.readyState === 1) {
			
			if (client.name && client.id && client.room_id !== null) {
				var pkthead2 = Buffer.alloc(3);
				pkthead2.writeUInt8(RPCID.USER_JOIN,0)
				pkthead2.writeUInt16LE(client.id,1)
				ws.send(Buffer.concat([pkthead2,client.name])) //stream the remote client to new client
				client.send(message) //stream to the remote client the new client
				
				if (client.room_id != 0) {
					var pktroom = Buffer.alloc(5);
					pktroom.writeUInt8(RPCID.CHANGE_ROOM,0)
					pktroom.writeUInt16LE(client.id,1)
					pktroom.writeUInt16LE(client.room_id,3)
					ws.send(pktroom) //Stream the room of some client not in lobby
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
		if (/*client !== ws &&*/ client.readyState === 1 && client.room_id == ws['room_id']) {
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
		if (/*client !== ws &&*/ client.readyState === 1 /*&& client.room_id == ws['room_id']*/) {
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