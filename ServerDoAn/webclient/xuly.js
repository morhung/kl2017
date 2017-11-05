var mlock;
var socket = io('/webclient');
socket.on('connect', function() {
  socket.emit('updateTrangThai') // cap nhat trang thai cua
})
socket.on("SVSEND", function(json){
  console.log("Nhan data tu server: ", json);
  var status = json.servo_stt;
  console.log("json.servo_stt: ", status);
  if (status == 1) {
    document.getElementById('status_box').innerHTML = "LOCK"
    document.getElementById('second_block_vertical').src = "img/lock.png"
    mlock = true;
  }else{
    document.getElementById('status_box').innerHTML = "UNLOCK"
    document.getElementById('second_block_vertical').src= "img/unlock.png"
    mlock = false;
  }
// console.log(status);
});
$(document).ready(function(){
  $('#button').click(function(){
    //updateServo(0);
    if (mlock) {
      updateServo(1);
    }else {
      updateServo(0);
    }
  });
})

function updateServo(x){
  console.log("Gui lenh khoa/mo khoa cua: ", x);
  var json = {
    "servo_stt": x
  }

  //var obj = JSON.stringify(json);
  //var obj = JSON.parse(json);
  socket.emit("SERVO", json)
}
