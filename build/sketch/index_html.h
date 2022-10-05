#line 1 "/Users/shin/Documents/Arduino/esp32_logger/index_html.h"
// index_html.h
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html><html><head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
<title>Sensor graph</title>
<link rel="shortcut icon" href="/espressif.ico" />
</head>
<div style="text-align:center;"><b>ADS1115</b></div>
<div class="chart-container" position: relative; height:350px; width:100%">
  <canvas id="myChart" width="600" height="400"></canvas>
</div>
<br><br>
<script src = "/Chart.min.js"></script>  
<script>
var graphData = {
  labels: [],  // X軸のデータ (時間)
  datasets: [{
                label: "Voltage",
                data: [], // Y humd
                fill: false,
                borderColor: "rgba(54, 162, 235, 0.2)",
                backgroundColor: "rgba(54, 162, 235, 1)",
            },
            {
                label: "Current",
                data: [], // Y pressure
                fill: false,
                borderColor: "rgba(255, 206, 86, 0.2)",
                backgroundColor: "rgba(255, 206, 86, 1)",
            },
            {
                label: "Power",
                data: [], // Y pressure
                fill: false,
                borderColor: "rgba(255, 99, 132, 0.2)",
                backgroundColor: "rgba(254,97,132,0.5)",
            }
  ]
};
var graphOptions = {
  maintainAspectRatio: false,
  scales: {
    yAxes: [{
      ticks: {beginAtZero:true}
    }]
  }
};
var ctx = document.getElementById("myChart").getContext('2d');
var chart = new Chart(ctx, {
  type: 'line',
  data: graphData,
  options: graphOptions
});
var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
ws.onmessage = function(evt) {
  var Time = new Date().toLocaleTimeString();
  var data_x1 = JSON.parse(evt.data)["voltage"];
  var data_x2 = JSON.parse(evt.data)["current"];
  var data_x3 = JSON.parse(evt.data)["power"];
  console.log(Time);
  console.log(data_x1);
  console.log(data_x2);
  console.log(data_x3);
  chart.data.labels.push(Time);
  chart.data.datasets[0].data.push(data_x1);
  chart.data.datasets[1].data.push(data_x2);
  chart.data.datasets[2].data.push(data_x3);
  chart.update();
};
ws.onclose = function(evt) {
  console.log("ws: onclose");
  ws.close();
}
ws.onerror = function(evt) {
  console.log(evt);
}
</script>
</body></html>
)=====";