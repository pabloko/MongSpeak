const SECRET = process.env.SECRET || "SecretChangeme";
var fs = require('fs');
const PORT = process.env.PORT || 80;
var dir = __dirname+'/uploads';
var use_virtual = false;
if (!fs.existsSync(dir))
    use_virtual = true
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
var fileList={};
var files = {};
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
/*todo: make this on static file, save across reboots*/
var known_clients=[]
var banned_guids=[]
var banned_ips=[]
var getWSByID = function(gd) {
	if (Array.from(wss.clients).filter((obj) => obj.id === gd).length > 0)
		return Array.from(wss.clients).filter((obj) => obj.id === gd)[0]
	return null;
}
var getKnownClientByGuid = function(gd) {
	if (known_clients.filter((obj) => obj.guid === gd).length > 0)
		return known_clients.filter((obj) => obj.guid === gd)[0]
	return null;
}
var getKnownClientByIP = function(gd) {
	if (known_clients.filter((obj) => obj.ip === gd).length > 0)
		return known_clients.filter((obj) => obj.ip === gd)[0]
	return null;
}

app.put('/upload/:filename', function(req, res) {
	var gd = req.header('user-mong')
	if (!gd) {
		res.status(500)
		res.end('nok');
		return;
	}
	if (Array.from(wss.clients).filter((obj) => obj.guid === gd).length == 0) {
		res.status(500)
		res.end('nok');
		return;
	}
	
	var tmpname = Math.random().toString(26).slice(2)+Math.random().toString(26).slice(2);
	var newname = tmpname+'_'+req.params.filename;
	fileList[newname]=gd;
	if (use_virtual) {
		req.on('data', (data)=>{
			if (!files[newname]) files[newname]=[]
			files[newname].push(data)
		})
		req.on('end', ()=> {
			res.end(newname)
		})
	} else {
		req.pipe(fs.createWriteStream(__dirname + '/uploads/'+newname, {flags: 'w', mode: 0666}));
		req.on('end', ()=> {
			res.end(newname)
		})
	}
	setTimeout(()=>{
		if (use_virtual)
			delete files[newname]
		else
			try { fs.unlinkSync(__dirname + '/uploads/'+newname); } catch(ex) {}
		delete fileList[newname]
	}, 1000 * 60 * 10) //10 min ttl
});

app.get('/file/:filename',function(req, res) {
	var filepath = __dirname + '/uploads/' + req.params.filename
	if (use_virtual) {
		if (files[req.params.filename]){
			files[req.params.filename].forEach((chunk)=>res.write(chunk))
			res.end()
		} else 
			res.end('404')
	} else {
		if (fs.existsSync(filepath)) {
			res.sendFile(filepath)
		} else 
			res.end('404')
	}
})

app.get('/admin/:pw/:action/:uid/:data',function(req, res) {
	//todo: auth
	
	if (req.params.pw!=SECRET) return res.end('')
	res.setHeader('Content-Type', 'text/html');
	var ws = getWSByID(parseInt(req.params.uid))
	if (ws) 
		switch (req.params.action) {
			case 'kick': {
				ws.close()
			} break;
			case 'move': {
				handle_room_change(parseInt(req.params.data),ws)
			} break;
			case 'dis_chat': {
				ws.dis_chat = parseInt(req.params.data)
			} break;
			case 'dis_voice': {
				ws.dis_voice = parseInt(req.params.data)
			} break;
			case 'dis_move': {
				ws.dis_move = parseInt(req.params.data)
			} break;
		}
	res.end('<meta http-equiv="refresh" content="0;URL=\'/admin/'+req.params.pw+'/\'" />')
})

app.get('/admin/:pw',function(req, res) {
	
	//todo: auth
	if (req.params.pw!=SECRET) return res.end('')
	res.setHeader('Content-Type', 'text/html');
	var room_list=""
	var room_options=''
	var user_list=""
	rooms.forEach((room)=>{
		room_list+='<tr><td>#'+room.id+' <BUTTON>REMOVE</BUTTON></td><td><input type="text" value="'+room.name+'" /></td><td><input type="text" /></td><td><input type="text" /></td><td><small>[<input type="checkbox" /> VOICE][<input type="checkbox" /> CHAT]</small></td><td><textarea></textarea></td></tr>'
		room_options+='<option value="'+room.id+'">'+room.name+'</option>'
	})
	wss.clients.forEach((client)=>{
		user_list+='<li>['+client.ipaddr+'] <b>'+(new TextDecoder('utf-16', { fatal: true }).decode(client.name).toString())+'</b> ('+client.id+') <i><small>'+client.guid+'</small></i><br>'
		user_list+='<button onclick="docommand(\'kick\','+client.id+',0)">KICK</button> | <select onchange="docommand(\'move\','+client.id+',this.value)"><option selected value="'+client.room_id+'">'+rooms[client.room_id].name+' [in use]</option>'+room_options+'</select> | Disallow: <small>[<input onchange="docommand(\'dis_voice\','+client.id+',this.checked?1:0)" type="checkbox" '+(client.dis_voice?'checked':'')+' /> VOICE] [<input onchange="docommand(\'dis_chat\','+client.id+',this.checked?1:0)" type="checkbox" '+(client.dis_chat?'checked':'')+' /> CHAT] [<input onchange="docommand(\'dis_move\','+client.id+',this.checked?1:0)" type="checkbox" '+(client.dis_move?'checked':'')+' /> ROOMCHANGE</small>]</li>'
	})
	var known_options=""
	known_clients.forEach((c)=>{
		known_options+='<option>'+c.name+' | '+c.ip+' | '+c.guid+'</option>'
	})
	
	var file_list=""
	Object.keys(fileList).forEach((k)=>{
		file_list+='<li><a href="/file/'+k+'" target=_blank>'+k+'</a><br>Uploaded by GUID: <small>'+fileList[k]+'</small></li>'
	})
	
	var html = `<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta http-equiv="X-UA-Compatible" content="IE=EDGE"/>
		<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css" />
		<link href='https://fonts.googleapis.com/css?family=Open+Sans' rel='stylesheet' type='text/css' />
		<style>
			*{
				box-sizing: border-box;
				font-family:'Open Sans';
				outline: 0;
			}
		</style>
	</head>
	<body><h1>MongSpeak Admin</h1><hr>
	User list:<br><ul>`+user_list+`</ul><hr>
	Rooms configuration:<br><table style="width:100%"><tr><td>#ID</td><td>ROOM NAME</td><td>MAX USERS</td><td>PASSWORD</td><td>DISALLOW</td><td>CUSTOM HTML</td></tr>`+room_list+`</table>
	<button>ADD ROOM</button> <button>SAVE ROOMS</button> <button>SAVE & BROADCAST TO ALL</button> 
	<hr>
	Banned users:
	<fieldset><legend>Known clients</legend><select multiple style="width:100%;">`+known_options+`</select><br><button>BAN IP</button><button>BAN GUID</button></fieldset>
	<fieldset style="float:right;width:48%"><legend>Banned IPs</legend><select multiple style="width:100%;"></select><br><button>UnBan</button><input type="text"/><button>MANUAL BAN IP</button></fieldset>
	<fieldset style="float:left;width:48%"><legend>Banned GUIDs</legend><select multiple style="width:100%;"></select><br><button>UnBan</button><input type="text"/><button>MANUAL BAN GUID</button></fieldset><hr>
	Files:<br>
	<ul>`+file_list+`</ul>
	<script>
	var docommand = function(a,b,c) {
		var path = window.location.pathname
		if (!path.endsWith('/')) path = path+'/'
		window.location.href=path+a+'/'+b+'/'+c
	}
	</script>
	</body>
	</html>`;
	
	res.end(html);
})

wss.on('connection', (ws,req) => {

	ws['id']=Math.floor((Math.random() * 9999) + 1111)
	while (Array.from(wss.clients).filter((obj) => obj.id === ws.id).length > 1) {
		ws['id']=Math.floor((Math.random() * 9999) + 1111)
	}
	var ip_addr = req.headers['x-forwarded-for'] || req.connection.remoteAddress.replace('::ffff:','');
	//console.log(ip_addr.replace('::ffff:',''));
	ws['ipaddr'] = ip_addr;
	var kickInactive = setTimeout(()=>{
		ws.close()
	},500)
    ws.on('message', (message) => {
		if (kickInactive && message[0] == RPCID.USER_INIT) {
			clearTimeout(kickInactive)
			kickInactive=null;
		}
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
	
	ws['dis_chat']=false
	ws['dis_voice']=false
	ws['dis_move']=false
	
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
	
	var kcli = getKnownClientByGuid(ws.guid)
	if (kcli==null)
		known_clients.push({
			name: new TextDecoder('utf-16', { fatal: true }).decode(ws.name).toString(),
			guid: ws['guid'],
			ip:   ws['ipaddr']
		})
})

join_rpc(RPCID.USER_INIT,(d, ws)=>{
	if (d.length!=36) {
		ws.close();
		return;
	}
	
	if (Array.from(wss.clients).filter((obj) => obj.guid === d.toString()).length > 0) {
		ws.close();
		return;
	}
	
	var roomsbuffer = Buffer.from(JSON.stringify(rooms), "ucs2")
	var rpcbuffer = new Buffer.alloc(3);
	rpcbuffer.writeUInt8(RPCID.ROOMS,0);
	rpcbuffer.writeUInt16LE(null,1);
	var finallbuffer = Buffer.concat([rpcbuffer,roomsbuffer])	
	ws.send(finallbuffer);
	
	ws['guid']=d.toString()
	ws['room_id'] = 0;
})

join_rpc(RPCID.USER_CHAT,(d, ws)=>{
	if (!ws['guid']) {ws.close(); return;}
	if (d.length==0) return;
	if (ws.dis_chat) return;
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
	if (ws.dis_voice) return;
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
	if (ws.dis_move) return;
	var room = d.readUInt16LE(0)
	handle_room_change(room,ws)
});

var handle_room_change = function(room,ws) {
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
}

function loadRooms(){
	roomsObject.forEach((room)=>{
		rooms.push(room);
	});
}

function initialize(){
	loadRooms();
}


initialize();