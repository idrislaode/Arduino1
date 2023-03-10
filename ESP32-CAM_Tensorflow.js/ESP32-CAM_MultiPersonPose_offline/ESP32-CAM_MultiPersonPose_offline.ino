/*
ESP32-CAM tfjs PoseNet offline
Author : ChungYi Fu (Kaohsiung, Taiwan)  2021-2-8 17:30
https://www.facebook.com/francefu

首頁
http://APIP
http://STAIP

自訂指令格式 :  
http://APIP/?cmd=P1;P2;P3;P4;P5;P6;P7;P8;P9
http://STAIP/?cmd=P1;P2;P3;P4;P5;P6;P7;P8;P9

預設AP端IP： 192.168.4.1
http://192.168.xxx.xxx?ip
http://192.168.xxx.xxx?mac
http://192.168.xxx.xxx?restart
http://192.168.xxx.xxx?digitalwrite=pin;value
http://192.168.xxx.xxx?analogwrite=pin;value
http://192.168.xxx.xxx?flash=value        //value= 0~255 閃光燈
http://192.168.xxx.xxx?getstill                 //取得視訊影像
http://192.168.xxx.xxx?getfile=/filename      //取得SD卡text/html檔
http://192.168.xxx.xxx?getimage=/filename     //取得SD卡image/jpeg檔
http://192.168.xxx.xxx?framesize=size     //size= UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA 改變影像解析度

查詢Client端IP：
查詢IP：http://192.168.4.1/?ip
重設網路：http://192.168.4.1/?resetwifi=ssid;password
*/

//輸入WIFI連線帳號密碼
const char* ssid     = "";   //your network SSID
const char* password = "";   //your network password

//輸入AP端連線帳號密碼
const char* apssid = "ESP32-CAM";
const char* appassword = "12345678";    //AP端密碼至少要八個字元以上

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"         //視訊
#include "soc/soc.h"            //用於電源不穩不重開機
#include "soc/rtc_cntl_reg.h"   //用於電源不穩不重開機
#include "FS.h"                 //file system wrapper
#include "SD_MMC.h"             //SD卡存取函式庫

String Feedback="";   //回傳客戶端訊息
//指令參數值
String Command="",cmd="",P1="",P2="",P3="",P4="",P5="",P6="",P7="",P8="",P9="";
//指令拆解狀態值
byte ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;

// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled

//安可信ESP32-CAM模組腳位設定
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiServer server(80);

void ExecuteCommand()
{
  //Serial.println("");
  //Serial.println("Command: "+Command);
  if (cmd!="getstill") {
    Serial.println("cmd= "+cmd+" ,P1= "+P1+" ,P2= "+P2+" ,P3= "+P3+" ,P4= "+P4+" ,P5= "+P5+" ,P6= "+P6+" ,P7= "+P7+" ,P8= "+P8+" ,P9= "+P9);
    Serial.println("");
  }
  
  if (cmd=="your cmd") {
    // You can do anything.
    // Feedback="<font color=\"red\">Hello World</font>";
  }
  else if (cmd=="ip") {
    Feedback="AP IP: "+WiFi.softAPIP().toString();    
    Feedback+=", ";
    Feedback+="STA IP: "+WiFi.localIP().toString();
  }  
  else if (cmd=="mac") {
    Feedback="STA MAC: "+WiFi.macAddress();
  }  
  else if (cmd=="resetwifi") {  //重設WIFI連線
    WiFi.begin(P1.c_str(), P2.c_str());
    Serial.print("Connecting to ");
    Serial.println(P1);
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;
    } 
    Serial.println("");
    Serial.println("STAIP: "+WiFi.localIP().toString());
    Feedback="STAIP: "+WiFi.localIP().toString();
  }    
  else if (cmd=="restart") {
    ESP.restart();
  }    
  else if (cmd=="digitalwrite") {
    ledcDetachPin(P1.toInt());
    pinMode(P1.toInt(), OUTPUT);
    digitalWrite(P1.toInt(), P2.toInt());
  }   
  else if (cmd=="analogwrite") {
    if (P1="4") {
      ledcAttachPin(4, 4);  
      ledcSetup(4, 5000, 8);   
      ledcWrite(4,P2.toInt());  
    }
    else {
      ledcAttachPin(P1.toInt(), 5);
      ledcSetup(5, 5000, 8);
      ledcWrite(5,P2.toInt());
    }
  }  
  else if (cmd=="flash") {
    ledcAttachPin(4, 4);  
    ledcSetup(4, 5000, 8);   
     
    int val = P1.toInt();
    ledcWrite(4,val);  
  }  
  else if (cmd=="framesize") { 
    sensor_t * s = esp_camera_sensor_get();  
    if (P1=="QQVGA")
      s->set_framesize(s, FRAMESIZE_QQVGA);
    else if (P1=="HQVGA")
      s->set_framesize(s, FRAMESIZE_HQVGA);
    else if (P1=="QVGA")
      s->set_framesize(s, FRAMESIZE_QVGA);
    else if (P1=="CIF")
      s->set_framesize(s, FRAMESIZE_CIF);
    else if (P1=="VGA")
      s->set_framesize(s, FRAMESIZE_VGA);  
    else if (P1=="SVGA")
      s->set_framesize(s, FRAMESIZE_SVGA);
    else if (P1=="XGA")
      s->set_framesize(s, FRAMESIZE_XGA);
    else if (P1=="SXGA")
      s->set_framesize(s, FRAMESIZE_SXGA);
    else if (P1=="UXGA")
      s->set_framesize(s, FRAMESIZE_UXGA);           
    else 
      s->set_framesize(s, FRAMESIZE_QVGA);     
  }   
  else {
    Feedback="Command is not defined.";
  }
  if (Feedback=="") Feedback=Command;  
}

//自訂網頁首頁管理介面
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
  <!DOCTYPE html>
  <head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">  
  <script src="?getfile=/tfjs@1.0.1"> </script>
  <script src="?getfile=/posenet.js"> </script>    
  </head><body>
  <img id="ShowImage" src="" style="display:none">
  <canvas id="canvas" width="0" height="0"></canvas>  
  <table>
  <tr>
    <td><input type="button" id="restart" value="Restart"></td> 
    <td colspan="2"><input type="button" id="getStill" value="Start Detect" style="display:none"></td> 
  </tr>
  <tr>
    <td>Object</td> 
    <td colspan="2">
      <select id="persons">
        <option value="1">1</option>
        <option value="2">2</option>
        <option value="3">3</option>
        <option value="4">4</option>
        <option value="5">5</option>
        <option value="999">No Limit</option>
      </select>
    </td>
  </tr>
  <tr>
    <td>ScoreLimit</td> 
    <td colspan="2">
      <select id="scorelimit">
        <option value="1.0">1</option>
        <option value="0.9">0.9</option>
        <option value="0.8">0.8</option>
        <option value="0.7">0.7</option>
        <option value="0.6">0.6</option>
        <option value="0.5">0.5</option>
        <option value="0.4">0.4</option>
        <option value="0.3">0.3</option>
        <option value="0.2">0.2</option>
        <option value="0.1">0.1</option>
        <option value="0" selected="selected">0</option>
      </select>
    </td>
  </tr>
  <tr>
    <td>MirrorImage</td> 
    <td colspan="2">  
      <select id="mirrorimage">
        <option value="1">yes</option>
        <option value="0">no</option>
      </select>
    </td>
  </tr>     
  <tr>
    <td>Resolution</td> 
    <td colspan="2">
      <select id="framesize">
        <option value="UXGA">UXGA(1600x1200)</option>
        <option value="SXGA">SXGA(1280x1024)</option>
        <option value="XGA">XGA(1024x768)</option>
        <option value="SVGA">SVGA(800x600)</option>
        <option value="VGA">VGA(640x480)</option>
        <option value="CIF">CIF(400x296)</option>
        <option value="QVGA" selected="selected">QVGA(320x240)</option>
        <option value="HQVGA">HQVGA(240x176)</option>
        <option value="QQVGA">QQVGA(160x120)</option>
      </select> 
    </td>
  </tr>    
  <tr>
    <td>Flash</td>
    <td colspan="2"><input type="range" id="flash" min="0" max="255" value="0"></td>
  </tr>
  <tr>
    <td>Quality</td>
    <td colspan="2"><input type="range" id="quality" min="10" max="63" value="10"></td>
  </tr>
  <tr>
    <td>Brightness</td>
    <td colspan="2"><input type="range" id="brightness" min="-2" max="2" value="0"></td>
  </tr>
  <tr>
    <td>Contrast</td>
    <td colspan="2"><input type="range" id="contrast" min="-2" max="2" value="0"></td>
  </tr>
  <tr><td>Rotate</td>
    <td align="left" colspan="2">
      <select onchange="document.getElementById('canvas').style.transform='rotate('+this.value+')';">
        <option value="0deg">0deg</option>
        <option value="90deg">90deg</option>
        <option value="180deg">180deg</option>
        <option value="270deg">270deg</option>
      </select>
    </td>
  </tr>  
  </table>
  <div id="result" style="color:red"><div>
  </body>
  </html> 
  
  <script>
    var getStill = document.getElementById('getStill');
    var ShowImage = document.getElementById('ShowImage');
    var canvas = document.getElementById("canvas");
    var context = canvas.getContext("2d"); 
    var object = document.getElementById('object');
    var score = document.getElementById("score");
    var mirrorimage = document.getElementById("mirrorimage");      
    var count = document.getElementById('count'); 
    var result = document.getElementById('result');
    var flash = document.getElementById('flash'); 
    var lastValue="";
    var myTimer;
    var restartCount=0;
    var Model;
    var imageScaleFactor = 0.75;
    
    getStill.onclick = function (event) {
      clearInterval(myTimer);  
      myTimer = setInterval(function(){error_handle();},5000);
      ShowImage.src=location.origin+'/?getstill='+Math.random();
    }
    
    function error_handle() {
      restartCount++;
      clearInterval(myTimer);
      if (restartCount<=2) {
        result.innerHTML = "Get still error. <br>Restart ESP32-CAM "+restartCount+" times.";
        myTimer = setInterval(function(){getStill.click();},10000);
      }
      else
        result.innerHTML = "Get still error. <br>Please close the page and check ESP32-CAM.";
    }    
    
    ShowImage.onload = function (event) {
      clearInterval(myTimer);
      restartCount=0;
      canvas.setAttribute("width", ShowImage.width);
      canvas.setAttribute("height", ShowImage.height);
      if (mirrorimage.value==1) {
        context.translate((canvas.width + ShowImage.width) / 2, 0);
        context.scale(-1, 1);
        context.drawImage(ShowImage, 0, 0, ShowImage.width, ShowImage.height);
        context.setTransform(1, 0, 0, 1, 0, 0);
      }
      else
        context.drawImage(ShowImage,0,0,ShowImage.width,ShowImage.height);
      if (Model) {
        DetectImage();
      }          
    }     
    
    restart.onclick = function (event) {
      fetch(location.origin+'/?restart=stop');
    }    
    
    framesize.onclick = function (event) {
      fetch(document.location.origin+'/?framesize='+this.value+';stop');
    }  
    
    flash.onchange = function (event) {
      fetch(location.origin+'/?flash='+this.value+';stop');
    } 
    
    quality.onclick = function (event) {
      fetch(document.location.origin+'/?quality='+this.value+';stop');
    } 
    
    brightness.onclick = function (event) {
      fetch(document.location.origin+'/?brightness='+this.value+';stop');
    } 
    
    contrast.onclick = function (event) {
      fetch(document.location.origin+'/?contrast='+this.value+';stop');
    }                             
    
    function ObjectDetect() {
      result.innerHTML = "Please wait for loading model.";
      posenet.load().then(function(net) {
        Model = net;
        result.innerHTML = "";
        getStill.style.display = "block";
      }); 
    }
    
    function DetectImage() {
      Model.estimatePoses(canvas, {flipHorizontal: false, decodingMethod: 'multi-person', maxPoseDetections: 5, scoreThreshold: 0.5, nmsRadius: 20}).then(pose => {
        //console.log(pose.score);
        //console.log(pose.keypoints);
        result.innerHTML = "";
        var scoreLimit = Number(document.getElementById("scorelimit").value);    
        var s = (canvas.width>canvas.height)?canvas.width:canvas.height;
 
        if (pose.length>0) {
          for (var n=0;n<pose.length;n++) {
          if (n<Number(document.getElementById("persons").value)) {
            var k = pose[n].keypoints;
            if (k.length>0) {
            for (var i=0;i<k.length;i++) {
              if (k[i].score>=scoreLimit) {
              const x = k[i].position.x;
              const y = k[i].position.y;
              context.fillStyle="#00FFFF";
              context.beginPath();
              context.arc(x, y, Math.round(s/200), 0,2*Math.PI);
              context.closePath();
              context.fill();
              }     
              result.innerHTML += n + "," + k[i].part + "," + Math.round(k[i].score*100)/100 + "," + Math.round(k[i].position.x) + "," + Math.round(k[i].position.y) + "<br>";
            }                
            
            context.lineWidth = 2;
            var centerShoulderX = (k[5].position.x+k[6].position.x)/2;
            var centerShoulderY = (k[5].position.y+k[6].position.y)/2; 
            if (k[5].score>=scoreLimit&&k[6].score>=scoreLimit) {
              context.beginPath();
              context.arc(centerShoulderX, centerShoulderY, 3, 0,2*Math.PI);
              context.closePath();
              context.fill();
            }              
            if (k[0].score>=scoreLimit) {
              context.strokeStyle = "#0000FF";
              context.beginPath();
              context.moveTo(k[0].position.x,k[0].position.y);
              context.lineTo(centerShoulderX, centerShoulderY);
              context.stroke(); 
            }
            if (k[5].score>=scoreLimit) {
              context.strokeStyle = "#FF3333";
              context.beginPath();
              context.moveTo(centerShoulderX, centerShoulderY);
              context.lineTo(k[5].position.x,k[5].position.y);
              context.stroke();
            }
            if (k[6].score>=scoreLimit) {
              context.strokeStyle = "#FF8800";
              context.beginPath();
              context.moveTo(centerShoulderX, centerShoulderY);
              context.lineTo(k[6].position.x,k[6].position.y);
              context.stroke();              
            }
            if (k[5].score>=scoreLimit&&k[7].score>=scoreLimit) {
              context.strokeStyle = "#FFCC22";
              context.beginPath();
              context.moveTo(k[5].position.x,k[5].position.y);
              context.lineTo(k[7].position.x,k[7].position.y);
              context.stroke();
            }
            if (k[6].score>=scoreLimit&&k[8].score>=scoreLimit) {
              context.strokeStyle = "#66DD00";
              context.beginPath();
              context.moveTo(k[6].position.x,k[6].position.y);
              context.lineTo(k[8].position.x,k[8].position.y);
              context.stroke();
            }
            if (k[7].score>=scoreLimit&&k[9].score>=scoreLimit) {
              context.strokeStyle = "#FFFF77";
              context.beginPath();
              context.moveTo(k[7].position.x,k[7].position.y);
              context.lineTo(k[9].position.x,k[9].position.y);
              context.stroke(); 
            }
            if (k[8].score>=scoreLimit&&k[10].score>=scoreLimit) {
              context.strokeStyle = "#BBFF66";
              context.beginPath();
              context.moveTo(k[8].position.x,k[8].position.y);
              context.lineTo(k[10].position.x,k[10].position.y);
              context.stroke();      
            }
            if (k[11].score>=scoreLimit) {
              context.strokeStyle = "#227700";
              context.beginPath();
              context.moveTo(centerShoulderX, centerShoulderY);
              context.lineTo(k[11].position.x,k[11].position.y);
              context.stroke(); 
            }
            if (k[12].score>=scoreLimit) {
              context.strokeStyle = "#9999FF";
              context.beginPath();
              context.moveTo(centerShoulderX, centerShoulderY);
              context.lineTo(k[12].position.x,k[12].position.y);
              context.stroke();    
            }
            if (k[11].score>=scoreLimit&&k[13].score>=scoreLimit) {
              context.strokeStyle = "#77FF00";
              context.beginPath();
              context.moveTo(k[11].position.x,k[11].position.y);
              context.lineTo(k[13].position.x,k[13].position.y);
              context.stroke(); 
            }
            if (k[12].score>=scoreLimit&&k[14].score>=scoreLimit) {
              context.strokeStyle = "#0066FF";
              context.beginPath();
              context.moveTo(k[12].position.x,k[12].position.y);
              context.lineTo(k[14].position.x,k[14].position.y);
              context.stroke();  
            }
            if (k[13].score>=scoreLimit&&k[15].score>=scoreLimit) {
              context.strokeStyle = "#99FF99";
              context.beginPath();
              context.moveTo(k[13].position.x,k[13].position.y);
              context.lineTo(k[15].position.x,k[15].position.y);
              context.stroke(); 
            }
            if (k[14].score>=scoreLimit&&k[16].score>=scoreLimit) {
              context.strokeStyle = "#0000CC";
              context.beginPath();
              context.moveTo(k[14].position.x,k[14].position.y);
              context.lineTo(k[16].position.x,k[16].position.y);
              context.stroke(); 
            }
            }
          }
          }
          
          var person_index = 0;
          if (mirrorimage.value==1) {
            var nose = posenet_person(person_index,"nose");
            var leftEye = posenet_person(person_index,"rightEye");
            var rightEye = posenet_person(person_index,"leftEye");
            var leftEar = posenet_person(person_index,"rightEar");
            var rightEar = posenet_person(person_index,"leftEar");
            var leftShoulder = posenet_person(person_index,"rightShoulder");
            var rightShoulder = posenet_person(person_index,"leftShoulder");
            var leftElbow = posenet_person(person_index,"rightElbow");
            var rightElbow = posenet_person(person_index,"leftElbow");
            var leftWrist = posenet_person(person_index,"rightWrist");
            var rightWrist = posenet_person(person_index,"leftWrist");
            var leftHip = posenet_person(person_index,"rightHip");
            var rightHip = posenet_person(person_index,"leftHip");
            var leftKnee = posenet_person(person_index,"rightKnee");
            var rightKnee = posenet_person(person_index,"leftKnee");
            var leftAnkle = posenet_person(person_index,"rightAnkle");
            var rightAnkle = posenet_person(person_index,"leftAnkle");
          }
          else {      
            var nose = posenet_person(person_index,"nose");
            var leftEye = posenet_person(person_index,"leftEye");
            var rightEye = posenet_person(person_index,"rightEye");
            var leftEar = posenet_person(person_index,"leftEar");
            var rightEar = posenet_person(person_index,"rightEar");
            var leftShoulder = posenet_person(person_index,"leftShoulder");
            var rightShoulder = posenet_person(person_index,"rightShoulder");
            var leftElbow = posenet_person(person_index,"leftElbow");
            var rightElbow = posenet_person(person_index,"rightElbow");
            var leftWrist = posenet_person(person_index,"leftWrist");
            var rightWrist = posenet_person(person_index,"rightWrist");
            var leftHip = posenet_person(person_index,"leftHip");
            var rightHip = posenet_person(person_index,"rightHip");
            var leftKnee = posenet_person(person_index,"leftKnee");
            var rightKnee = posenet_person(person_index,"rightKnee");
            var leftAnkle = posenet_person(person_index,"leftAnkle");
            var rightAnkle = posenet_person(person_index,"rightAnkle");
          }
          var noseLeft = Number(nose[3]);
          var noseTop = Number(nose[4]);
          var leftEyeLeft = Number(leftEye[3]);
          var leftEyeTop = Number(leftEye[4]);
          var rightEyeLeft = Number(rightEye[3]);
          var rightEyeTop = Number(rightEye[4]);
          var leftEarLeft = Number(leftEar[3]);
          var leftEarTop = Number(leftEar[4]);
          var rightEarLeft = Number(rightEar[3]);
          var rightEarTop = Number(rightEar[4]);
          var leftShoulderLeft = Number(leftShoulder[3]);
          var leftShoulderTop = Number(leftShoulder[4]);
          var rightShoulderLeft = Number(rightShoulder[3]);
          var rightShoulderTop = Number(rightShoulder[4]);
          var leftElbowLeft = Number(leftElbow[3]);
          var leftElbowTop = Number(leftElbow[4]);
          var rightElbowLeft = Number(rightElbow[3]);
          var rightElbowTop = Number(rightElbow[4]);
          var leftWristLeft = Number(leftWrist[3]);
          var leftWristTop = Number(leftWrist[4]);
          var rightWristLeft = Number(rightWrist[3]);
          var rightWristTop = Number(rightWrist[4]);
          var leftHipLeft = Number(leftHip[3]);
          var leftHipTop = Number(leftHip[4]);
          var rightHipLeft = Number(rightHip[3]);
          var rightHipTop = Number(rightHip[4]);
          var leftKneeLeft = Number(leftKnee[3]);
          var leftKneeTop = Number(leftKnee[4]);
          var rightKneeLeft = Number(rightKnee[3]);
          var rightKneeTop = Number(rightKnee[4]);
          var leftAnkleLeft = Number(leftAnkle[3]);
          var leftAnkleTop = Number(leftAnkle[4]);
          var rightAnkleLeft = Number(rightAnkle[3]);
          var rightAnkleTop = Number(rightAnkle[4]);

        }
        else {
          result.innerHTML = "Unrecognizable";
        }
    
        try { 
          document.createEvent("TouchEvent");
          setTimeout(function(){getStill.click();},250);
        }
        catch(e) { 
          setTimeout(function(){getStill.click();},150);
        } 
      });
    }
    
    window.onload = function () { ObjectDetect(); } 

    function posenet_person(input_person, input_part){
      var scoreLimit = Number(document.getElementById("scorelimit").value);
      var result = document.getElementById("result").innerHTML.split("<br>");
      if (result.length>0) {
      for (var i=0;i<result.length;i++) {
        var result_detail = result[i].split(",");
        if (result_detail[0]==input_person&&result_detail[1]==input_part&&Number(result_detail[2])>=scoreLimit) 
        return result_detail;
      }
      }
      return "-1,-1,-1,-1,-1";
    } 
    
    function position_angle(input_x0,input_y0,input_x1,input_y1) {
      return (Math.atan((input_y1-input_y0)/(input_x1-input_x0)) / Math.PI) * 180;
    }         
  </script>  
)rawliteral";

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //關閉電源不穩就重開機的設定
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);  //開啟診斷輸出
  Serial.println();

  //視訊組態設定
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);  //UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA  設定初始化影像解析度

  //SD Card偵測
  if(!SD_MMC.begin()){
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD_MMC card attached");
    SD_MMC.end();
    return;
  }
  Serial.println("");
  Serial.print("SD_MMC Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }  
  Serial.printf("SD_MMC Card Size: %lluMB\n", SD_MMC.cardSize() / (1024 * 1024));
  Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
  Serial.println();
  
  SD_MMC.end();   

  //閃光燈
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);    
  
  WiFi.mode(WIFI_AP_STA);
  
  //指定Client端靜態IP
  //WiFi.config(IPAddress(192, 168, 201, 100), IPAddress(192, 168, 201, 2), IPAddress(255, 255, 255, 0));

  WiFi.begin(ssid, password);    //執行網路連線

  delay(1000);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if ((StartTime+10000) < millis()) break;    //等待10秒連線
  } 

  if (WiFi.status() == WL_CONNECTED) {    //若連線成功
    WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);   //設定SSID顯示客戶端IP         
    Serial.println("");
    Serial.println("STAIP address: ");
    Serial.println(WiFi.localIP());  

    for (int i=0;i<5;i++) {   //若連上WIFI設定閃光燈快速閃爍
      ledcWrite(4,10);
      delay(200);
      ledcWrite(4,0);
      delay(200);    
    }         
  }
  else {
    WiFi.softAP((WiFi.softAPIP().toString()+"_"+(String)apssid).c_str(), appassword);         

    for (int i=0;i<2;i++) {    //若連不上WIFI設定閃光燈慢速閃爍
      ledcWrite(4,10);
      delay(1000);
      ledcWrite(4,0);
      delay(1000);    
    }
  }     

  //指定AP端IP    
  //WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)); 
  Serial.println("");
  Serial.println("APIP address: ");
  Serial.println(WiFi.softAPIP());    

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);  

  server.begin();          
}

void loop() {
  Feedback="";Command="";cmd="";P1="";P2="";P3="";P4="";P5="";P6="";P7="";P8="";P9="";
  ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;
  
   WiFiClient client = server.available();

  if (client) { 
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();             
        
        getCommand(c);   //將緩衝區取得的字元拆解出指令參數
                
        if (c == '\n') {
          if (currentLine.length() == 0) {    
            
            if (cmd=="getstill") {
              //回傳JPEG格式影像
              camera_fb_t * fb = NULL;
              fb = esp_camera_fb_get();  
              if(!fb) {
                Serial.println("Camera capture failed");
                delay(1000);
                ESP.restart();
              }
  
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Origin: *");              
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: image/jpeg");
              client.println("Content-Disposition: form-data; name=\"imageFile\"; filename=\"picture.jpg\""); 
              client.println("Content-Length: " + String(fb->len));             
              client.println("Connection: close");
              client.println();
              
              uint8_t *fbBuf = fb->buf;
              size_t fbLen = fb->len;
              for (size_t n=0;n<fbLen;n=n+1024) {
                if (n+1024<fbLen) {
                  client.write(fbBuf, 1024);
                  fbBuf += 1024;
                }
                else if (fbLen%1024>0) {
                  size_t remainder = fbLen%1024;
                  client.write(fbBuf, remainder);
                }
              }  
              
              client.println();
              esp_camera_fb_return(fb);
            
              pinMode(4, OUTPUT);
              digitalWrite(4, LOW);               
            }
            else if (cmd=="getfile") {           
              //回傳SD卡js檔案
              if(!SD_MMC.begin()){
                Serial.println("Card Mount Failed");
              }  
              
              fs::FS &fs = SD_MMC;
              File file = fs.open(P1);
              if(!file){
                Serial.println("Failed to open file for reading");
                SD_MMC.end();    
              }
              else {
                Serial.println("Read from file: "+P1);
                Serial.println("file size: "+String(file.size()));            

                client.println("HTTP/1.1 200 OK");
                client.println("Access-Control-Allow-Origin: *");              
                client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
                client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
                client.println("Content-Type: text/html");
                P1.replace("/","");
                client.println("Content-Disposition: form-data; name=\"jsFile\"; filename=\""+P1+"\""); 
                client.println("Content-Length: " + String(file.size()));             
                client.println("Connection: close");
                client.println();

                byte buf[1024];
                int i = -1;
                while (file.available()) {
                  i++;
                  buf[i] = file.read();
                  if (i==(sizeof(buf)-1)) {
                    client.write((const uint8_t *)buf, sizeof(buf));
                    i = -1;
                  }
                  else if (!file.available())
                    client.write((const uint8_t *)buf, (i+1));
                }

                file.close();
                SD_MMC.end();
              }
            
              pinMode(4, OUTPUT);
              digitalWrite(4, LOW);               
            }  
            else if (cmd=="getimage") {           
              //回傳SD卡影像檔案
              if(!SD_MMC.begin()){
                Serial.println("Card Mount Failed");
              }  
              
              fs::FS &fs = SD_MMC;
              File file = fs.open(P1);
              if(!file){
                Serial.println("Failed to open file for reading");
                SD_MMC.end();    
              }
              else {
                Serial.println("Read from file: "+P1);
                Serial.println("file size: "+String(file.size()));            

                client.println("HTTP/1.1 200 OK");
                client.println("Access-Control-Allow-Origin: *");              
                client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
                client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
                client.println("Content-Type: image/jpeg");
                P1.replace("/","");
                client.println("Content-Disposition: form-data; name=\"imageFile\"; filename=\""+P1+"\""); 
                client.println("Content-Length: " + String(file.size()));             
                client.println("Connection: close");
                client.println();

                byte buf[1024];
                int i = -1;
                while (file.available()) {
                  i++;
                  buf[i] = file.read();
                  if (i==(sizeof(buf)-1)) {
                    client.write((const uint8_t *)buf, sizeof(buf));
                    i = -1;
                  }
                  else if (!file.available())
                    client.write((const uint8_t *)buf, (i+1));
                }

                file.close();
                SD_MMC.end();
              }
            
              pinMode(4, OUTPUT);
              digitalWrite(4, LOW);               
            }              
            else {
              //回傳HTML首頁或Feedback
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: text/html; charset=utf-8");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Connection: close");
              client.println();
              
              String Data="";
              if (cmd!="")
                Data = Feedback;
              else {
                Data = String((const char *)INDEX_HTML);
              }
              int Index;
              for (Index = 0; Index < Data.length(); Index = Index+1000) {
                client.print(Data.substring(Index, Index+1000));
              }           
              
              client.println();
            }
                        
            Feedback="";
            break;
          } else {
            currentLine = "";
          }
        } 
        else if (c != '\r') {
          currentLine += c;
        }

        if ((currentLine.indexOf("/?")!=-1)&&(currentLine.indexOf(" HTTP")!=-1)) {
          if (Command.indexOf("stop")!=-1) {  //若指令中含關鍵字stop立即斷線 -> http://192.168.xxx.xxx/?cmd=aaa;bbb;ccc;stop
            client.println();
            client.println();
            client.stop();
          }
          currentLine="";
          Feedback="";
          ExecuteCommand();
        }
      }
    }
    delay(1);
    client.stop();
  }
}

//拆解命令字串置入變數
void getCommand(char c)
{
  if (c=='?') ReceiveState=1;
  if ((c==' ')||(c=='\r')||(c=='\n')) ReceiveState=0;
  
  if (ReceiveState==1)
  {
    Command=Command+String(c);
    
    if (c=='=') cmdState=0;
    if (c==';') strState++;
  
    if ((cmdState==1)&&((c!='?')||(questionstate==1))) cmd=cmd+String(c);
    if ((cmdState==0)&&(strState==1)&&((c!='=')||(equalstate==1))) P1=P1+String(c);
    if ((cmdState==0)&&(strState==2)&&(c!=';')) P2=P2+String(c);
    if ((cmdState==0)&&(strState==3)&&(c!=';')) P3=P3+String(c);
    if ((cmdState==0)&&(strState==4)&&(c!=';')) P4=P4+String(c);
    if ((cmdState==0)&&(strState==5)&&(c!=';')) P5=P5+String(c);
    if ((cmdState==0)&&(strState==6)&&(c!=';')) P6=P6+String(c);
    if ((cmdState==0)&&(strState==7)&&(c!=';')) P7=P7+String(c);
    if ((cmdState==0)&&(strState==8)&&(c!=';')) P8=P8+String(c);
    if ((cmdState==0)&&(strState>=9)&&((c!=';')||(semicolonstate==1))) P9=P9+String(c);
    
    if (c=='?') questionstate=1;
    if (c=='=') equalstate=1;
    if ((strState>=9)&&(c==';')) semicolonstate=1;
  }
}
