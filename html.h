// SYNTAX: R"DELIMETER( html code in here )DELIMETER"; 
// R"( ... )";                                Most basic delimeter you can do
// R"=====( ... )=====";                      This delimeter is less likely to conflict with html code
//
// BASIC HTML Template:
// 
// <!DOCTYPE html>
// <html lang="en" class="js-focus-visible">
//   <head>
//     <title>Webpage Title Here</title>
//     <style> 
//       Define how your object looks here
//     </style>
//   </head>
// 
//   <body>
//     Instantiate your objects here
//   </body>
// 
//   <script type = "text/javascript">
//     Create javascript functions here
//   </script>
// </html>


const char MAIN_HTML[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang="en" class="js-focus-visible">
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32_INDOOR</title>
    <style>

    td {
      padding: 10px;
    }

    .clock-container {
      display: flex;
      justify-content: center;
      align-items: center;
      margin-bottom: 20px;
    } .clock-container .clock {
      text-align: center;
      font-size: 18px;
      background-color: gray;
      border: 1px solid #000;
      padding: 5px;
      margin-right: 30px;
      width: 40%;
    }

    .slider-container {
      display: flex;
      justify-content: center;
      align-items: center;
      margin-bottom: 60px
    } .slider-container .slider {
      margin-right: 30px;
      width: 40%;
    }

    .func-btn-container{
      margin-left: auto;
      margin-right: auto;
      width: 50%;
    } .func-btn-container .button {
      width:  100%;
      height: 30px;
      font-size: 16px;
      background-color: rgb(129, 168, 186);
    }

    .button-container {
      margin-left: auto;
      margin-right: auto;
      width: 50%;
    } .button-container .button {
      width:  100%;
      height: 50px;
      font-size: 20px;
      background-color: gray;
    }

    </style>
  </head>
  
  
  <body onload="main_setup()">

    <div class="clock-container">
      <div class="clock" id="clock">00:00</div>
      <button id="sync_btn" onclick="on_button_press(event)">Sync</button>
    </div>

    <div class="slider-container">
      <input  class="slider" id="slider" type="range" min="0" max="4096" value="0" onmouseup="on_button_press(event)" ontouchend="on_button_press(event)">
      <button id="unused-btn" onclick="on_button_press(event)">unused</button>
    </div>
    
    <table class="func-btn-container">
      <tr>
        <td><button class="button" id="rx_btn" onclick="on_button_press(event)">receive(example)</button></td>
        <td><button class="button" id="tx_btn" onclick="on_button_press(event)">transmit(example)</button></td>
      </tr>
    </table>

    <table class="button-container">
      <tr>
        <td><button class="button" id="btn0" onclick="on_button_press(event)">0</button></td>
        <td><button class="button" id="btn1" onclick="on_button_press(event)">1</button></td>
      </tr>
      <tr>
        <td><button class="button" id="btn2" onclick="on_button_press(event)">2</button></td>
        <td><button class="button" id="btn3" onclick="on_button_press(event)">3</button></td>
      </tr>
      <tr>
        <td><button class="button" id="btn4" onclick="on_button_press(event)">4</button></td>
        <td><button class="button" id="btn5" onclick="on_button_press(event)">5</button></td>
      </tr>
      <tr>
        <td><button class="button" id="btn6" onclick="on_button_press(event)">6</button></td>
        <td><button class="button" id="btn7" onclick="on_button_press(event)">7</button></td>
      </tr>
    </table>

  </body>
  
  


  <script type = "text/javascript">

    var global_data = {
      // Initial value is uint8_t(-1) == 0xFF, uint16_t(-1) == 0xFFFF
      "hr"  : 0xFF,       
      "min" : 0xFF,
      "btn" : 0xFFFF,
      "pwm" : 0xFFFF,       
    };


    // #########################################################################
    // MAIN
    // #########################################################################
    // GLOBAL VARIABLES
    var xhttp = new XMLHttpRequest();

    function main_setup(){


      setInterval(main_loop, 1000);
    }

    function main_loop(){
      var global_data_buf = global_data;

      // Request server for updated data
      sync_data(global_data_buf);
    }



    // #########################################################################
    // FUNCTIONS
    // #########################################################################
    function sync_data(client_data){
      if(typeof client_data !== 'object' || client_data === null){
        console.error('client_data must be an object type');
        return;
      }

      fetch('/sync', {
          method:  'POST', // or 'PUT'
          headers: {'Content-Type': 'application/json'},
          body:    JSON.stringify(client_data),
        })
        .then (response => {
          // Only parse as JSON from server if the response is not empty
          if (response.headers.get('content-length') > 0) 
            return response.json();
          else 
            return {};
        })
        .then (data  => process_sync_data(data)       )
        .catch(error => console.error('Error:', error));
    }


    function s_recv_data(){
      fetch('/m2s_data', {                                   // send request to /m2s_data
          method:  'POST', 
          headers: {'Content-Type': 'application/json'},
          body:    "{}",
        })
        .then (response => {                                 // reads reponse as json
          // Only parse as JSON from server if the response is not empty
          if (response.headers.get('content-length') > 0) 
            return response.json();
          else 
            return {};
        })                     
        .then (data  => process_recieved_data(data)   )      // copy json's key:val into server_data
        .catch(error => console.error('Error:', error));     // check for errors
    }


    function s_send_data(client_data){
      if(typeof client_data !== 'object' || client_data === null){
        console.error('client_data must be a dictionary (object) type');
        return;
      }
      // client_data["unix_time"] = Date.now();

      fetch('/s2m_data', {
          method: 'POST', // or 'PUT'
          headers: {'Content-Type': 'application/json'},
          body:    JSON.stringify(client_data),
        })
        .then (response => {
          // Only parse as JSON from server if the response is not empty
          if (response.headers.get('content-length') > 0) 
            return response.json();
          else 
            return {};
        })
        .then (data   => {console.log('Response:', data);})
        .catch(error  => {console.error('Error:', error);});
    }

    // BUTTON PRESS handler ###################################################
    function on_button_press(event) {
      var btn_id = event.target.id;
      var client_data = {};

      // Example data to send to server.
      // You can change this to any size and values you want
      var example_data = {
        myInt:    123,
        myFloat:  9.25,
        myString: "qwerty",
        sdata:    [99, 8.8, "qq"]
      };
 
      // Handling misc buttons ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      switch(btn_id){
        case "rx_btn":   s_recv_data();             return;
        case "tx_btn":   s_send_data(example_data); return;
        case "sync_btn": client_data = get_time();   break;
        case "slider":   client_data["pwm"] = document.getElementById("slider").value;  break;
        case "unused-btn": return;  // a dummy button. has no functionality
      }

      // Handling btn0-btn7 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      switch(btn_id){
        case "btn0": client_data["btn"] =  1<<0; break;
        case "btn1": client_data["btn"] =  1<<1; break;
        case "btn2": client_data["btn"] =  1<<2; break;
        case "btn3": client_data["btn"] =  1<<3; break;
        case "btn4": client_data["btn"] =  1<<4; break;
        case "btn5": client_data["btn"] =  1<<5; break;
        case "btn6": client_data["btn"] =  1<<6; break;
        case "btn7": client_data["btn"] =  1<<7; break;
      } 

      // console.log("s_send_data()> client_data: " + JSON.stringify(client_data));

      // Instantlly update locally on client side for more responsiveness
      // These local updates will be overwritten once we receive the server's response
      // Try commenting update_local_values() out and see how slow the graphical button reponse is.
      if(Object.keys(client_data).length !== 0)
        update_local_values(client_data);

      // Requests server for new values
      if(Object.keys(client_data).length !== 0)
        s_send_data(client_data); return;
    }


    function get_time(){
      var date = new Date();
      var time_obj = {
        "g_time": "",
        "hr"  : date.getHours(),
        "min" : date.getMinutes(),
        "sec" : date.getSeconds()
      }
      return time_obj;
    }

    // SYNC DATA helper functions ###################################################
    function process_sync_data(server_data){
      if(Object.keys(server_data).length === 0)
        return;

      set_clock_value(server_data);
      set_buttons_value(server_data);
      set_pwm_value(server_data)
    }

    function set_clock_value(server_data){
      // console.log("set_clock_value: " + JSON.stringify(server_data));
      let is_time_obj = false;
      if(server_data.hasOwnProperty('hr')){
        global_data["hr"] = server_data["hr"];
        is_time_obj = true;
      }
      if(server_data.hasOwnProperty('min')){
        global_data["min"] = server_data["min"];
        is_time_obj = true;
      }

      if(!is_time_obj)
        return;
      
      hr_str  = global_data["hr"] .toString().padStart(2, '0');
      min_str = global_data["min"].toString().padStart(2, '0');
      document.getElementById("clock").innerHTML = hr_str + ":" + min_str;

      if(document.getElementById("clock").style.backgroundColor !== "lightgray")
        document.getElementById("clock").style.backgroundColor = "lightgray";
    }

    function set_buttons_value(server_data){
      if(!server_data.hasOwnProperty('btn'))
        return;
        
      global_data["btn"] = server_data["btn"]

      // Flag Bits: 1=Green, 0=Lightgray
      for(let i=0; i<8; i++){
        let btn_name = "btn" + i;
        if(global_data["btn"] & 1<<i)
          document.getElementById(btn_name).style.backgroundColor = "green";
        else
          document.getElementById(btn_name).style.backgroundColor = "lightgray";
      }
    }

    function set_pwm_value(server_data){
      if(!server_data.hasOwnProperty('pwm'))
        return;

      global_data["pwm"] = server_data["pwm"];
      document.getElementById("slider").value = global_data["pwm"];
    }

    // SEND DATA helper functions ##################################################
    function update_local_values(client_data){
    // This function is meant to update the button's graphic instantly
    // so we don't have to wait for the polling response from the server.
    // This will make the user experience more responsive.
    // The server's response will overwrite these changes of course.
      if(client_data.hasOwnProperty('btn'))
        update_local_buttons(client_data["btn"]);
      
    }

    function update_local_buttons(btn_flag_bit){
      // We dont want to update locally if we still have default value.
      // That means we haven't synced to the server once since bootup
      if(global_data["btn"] == 0xFFFF)
        return;

      // Flag Bits: 1=Green, 0=Lightgray
      for(let i=0; i<8; i++){
        if(btn_flag_bit == (1<<i)){
          let btn_name      = "btn" + i;
          let cur_btn_color = document.getElementById(btn_name).style.backgroundColor;

          if(cur_btn_color === "green")
            document.getElementById(btn_name).style.backgroundColor = "lightgray";
          else
            document.getElementById(btn_name).style.backgroundColor = "green";
          return;
        }
      }
    }


    // RECIEVE DATA helper functions ##################################################
    function process_recieved_data(server_data){
      console.log("process_recieved_data: " + JSON.stringify(server_data));
      if(Object.keys(server_data).length === 0){
        console.log("Received empty json object!");
        return;
      }
    }

  </script>
</html>



)=====";