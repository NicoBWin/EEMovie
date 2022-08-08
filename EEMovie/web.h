const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title> Electroestimulador </title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  body {
    margin:5%;
    background-color:white;
  }
  html {font-family: Arial; display: inline-block;  margin: 0px auto;}
  .topnav {overflow: hidden;background-color: #0A1128;}
  img {display: block;margin-left: auto;margin-right: auto;}
  div{
    background-color: #FFFFFF;
    margin: 5%;
    /* border: 3px solid black; */
    padding: 2.5%;
  }
  div.content{
    position: relative;
    height: 200px;
  }
  div.card_s{
    margin: 5%;
    padding: 2.5%;
    border: 3px solid black;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
  }
  div.card{
    position: absolute;
    margin: 0%;
    top:0;
    left:0;
    width: 94%;
    border: 3px solid black;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
  }
  .card_title{
    text-align: center;
    font-size: 1.2rem;
    font-weight: bold;
    color: #b30000;
  }
  .watch{
    text-align: center;
    font-size: 1.2rem;
    height: 3rem;
    line-height: 3rem;
    color:#000000;
  }
  .ds-labels{
    font-size: 1.5rem;
    line-height: 2rem;
    height: 2rem;
    vertical-align:middle;
    padding-bottom: 15px;
  }
  .input_slider{
    width:90%;
    margin-left: 5%;
    margin-right: 5%;
  }
    .switch {position: relative; display: inline-block; width: 4.8rem; height: 2.4rem} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 2rem; width: 2rem; left: 0.2rem; bottom: 0.2rem; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(2.4rem); -ms-transform: translateX(2.4rem); transform: translateX(2.4rem)}
  </style>
</head>
<body>

  <div align=center>
    <h1 style="color:#b30000;font-size:25px;">Electro-estimulador</h1>
  <div/>
  
  <div id="ds_state_card" class="card_s" align=center>
    <div class="card-title">
        <span style="color:#b30000;font-size:20px;font-weight:bold;">Estado</span>
    </div>
    <div class="ds-labels"><span id="ds_state"></span></div>
    <div class="watch">
        <span id="watch_state"></span>
    </div>
    <div class="ctrl_sw" id="onctrl">
      <label class="switch">
        <input type="checkbox" id="sw_ctrl" onchange="toggle_ctrl(this)">
        <span class="slider"></span>
      </label>
    </div>
  </div>

  <div class="content">
    <div id="card_freq" class="card" align=center>
      <div class="card_title">
        Frecuencia
      </div>
      <div class="watch">
        <span id="watch_f"></span>Hz
      </div>
      <div>
        <input type="range" id="slider_f" oninput="updateFreqSlider(this)" min="0" max="4" step="0.1" value ="0" class="input_slider">
      </div>
    </div>
  </div>

</body>

<script>

var card_f = document.getElementById("card_freq");
var slider_f = document.getElementById("slider_f");
var watch_f = document.getElementById("watch_f");

watch_f.innerHTML = slider_f.value;

var ds_state = document.getElementById("ds_state");
var watch_state = document.getElementById("watch_state");
var sw_ctrl = document.getElementById("sw_ctrl");

watch_state.innerHTML = "APAGADO";

// sw_ctrl.disabled = true;

function toggle_ctrl(element) {
  if(element.checked){
    watch_state.innerHTML = "ENCENDIDO"
  }
  else{
    watch_state.innerHTML = "APAGADO"
  }

  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/checkbox?value="+element.checked, true);
  xhr.send();
}


function updateFreqSlider(element) {
  var sliderVal = Math.floor(Math.pow(10, +element.value));

  watch_f.innerHTML = sliderVal;
  
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider_f?value="+sliderVal, true);
  xhr.send();
}


var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
      var key = keys[i];
      if(key == "ds_state")
      {
        ds_temp.innerHTML = myObj[key];
      }
      else if(key == "ds_bool")
      {
        if(myObj[key]=="1")  //  Sensor not connected
        {
          sw_ctrl.disabled = true;
          sw_ctrl.checked = false;
          toggle_ctrl(sw_ctrl);
          
        }
        else  //  Sensor connected
        {
          sw_ctrl.disabled = false;
        }
      }
      else if( key == "sw_ctrl" )
      {
        if(myObj[key] == "false")
        {
          sw_ctrl.checked = false;
          
        }
        else if(myObj[key] == "true")
        {
          sw_ctrl.checked = true;
        }
      }
      else if( key == "slider_f" )
      {
        var element = document.getElementById(key);
        //console.log(element.id);
        
        var sliderID = element.id.charAt(element.id.length-1);
        var watch = document.getElementById("watch_"+sliderID);
        element.value = myObj[key];
        watch.innerHTML = myObj[key];
      }
    }
}
</script>
</html>)rawliteral";
