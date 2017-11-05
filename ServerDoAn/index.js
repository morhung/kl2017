var fs = require('fs');
var http = require('http');
var express = require('express');
var socketio = require('socket.io');
var ip = require('ip');
var app = express();
var server = http.Server(app)
var io = socketio(server);

var webclient_nsp = io.of('/webclient')
var esp8266_nsp = io.of('/esp8266')
var androidapp_nsp = io.of('/androidapp')
var middleware = require('socketio-wildcard')();

esp8266_nsp.use(middleware);
webclient_nsp.use(middleware);
androidapp_nsp.use(middleware);

PORT = 3638
server.listen(process.env.PORT || PORT);
console.log("Server started at: " + ip.address() +":" + PORT);


app.use(express.static("node_modules/socket.io-client")) 				// Có thể truy cập các file trong node_modules/socket.io-client từ xa
app.use(express.static("node_modules/socket.io"))
app.use(express.static("webclient"))

app.set("view engine", "ejs");
app.set("views", "./webclient");
app.get("/", function(req, res){
  res.render("trangchu");
});

function ParseJson(jsondata) {
    try {
        return JSON.parse(jsondata);
    } catch (error) {
        return null;
    }
}

esp8266_nsp.on('connection', function(socket) {
	console.log('ESP8266 da ket noi. !!')
	socket.on('disconnect', function() {
		console.log("ESP8266 mat ket noi.")
	})
	//nhận được bất cứ lệnh nào
	socket.on("*", function(packet) {
		console.log("esp8266 gui data toi client: ", packet.data) //in ra để debug

		var eventName = packet.data[0]
		var eventJson = packet.data[1] || {} //nếu gửi thêm json thì lấy json từ lệnh gửi, không thì gửi chuỗi json rỗng, {}
    //console.log("eventName (lenh) gui toi server va client: " + eventName);
    //console.log("eventJson la: " + eventJson);
    writeAsync(eventJson); // Lưu trạng thái của cửa vào file.
		 //gửi toàn bộ lệnh + json đến client
    webclient_nsp.emit(eventName, eventJson)
    androidapp_nsp.emit(eventName, eventJson) // gửi toàn bộ lệnh + json đến app android.
	})
})


webclient_nsp.on('connection', function(socket){
  console.log('WEB client da ket noi..!!');
  socket.on('disconnect', function(){
    console.log('WEB client mat ket noi.');
  })
  socket.on('*', function(packet){
    //var x = JSON.stringify(packet.data);
    console.log("WEB client gui data toi ESP8266:  " + packet.data);

    var eventName = packet.data[0]
    var eventJson = packet.data[1] || {}
    //console.log("eventName (lenh) webclient gui la: " + eventName);
    //console.log("eventJson la: " + eventJson);
    if (eventName == "updateTrangThai") {
      readAsync();
    }else {
      esp8266_nsp.emit(eventName, eventJson)
    }
  })
})

androidapp_nsp.on('connection', function(socket){
  console.log('Android client da ket noi. !!')
  socket.on('disconnect', function(){
    console.log('Android client mat ket noi.');
  })
  socket.on('*', function(packet){
    console.log("Android client gui data toi ESP8266: ", packet.data)
    var eventName = packet.data[0]
    var eventJson = packet.data[1] || {}
    //console.log("eventName (lenh) laf: " + eventName);
    //console.log("eventJson la: " + eventJson);
    if (eventName == "updateTrangThai") {
      //Read file database.json
      readAsync();
    }else {
        esp8266_nsp.emit(eventName, eventJson)
    }

  });
})

function readAsync(){
  fs.readFile("status.json", 'utf8', function(err, data){
    if (err) {
      throw err;
    }
    var obj = JSON.parse(data);
    webclient_nsp.emit("SVSEND", obj);
    androidapp_nsp.emit("SVSEND", obj);
  })
}

function writeAsync(x){
  fs.writeFile("status.json",JSON.stringify(x), "utf8", function(err){
    if (err) {
      throw err;
      console.log("fileAsync complete");
    }
  })
}
