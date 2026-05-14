// html.h
#pragma once

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Controle de LED</title>
  <style>
    body { font-family: sans-serif; background: #0f0f0f; color: #fff;
           display: flex; flex-direction: column; align-items: center;
           justify-content: center; min-height: 100vh; gap: 24px; }
    .day  { color: #C9DB00; } .night { color: #8300DB; }
    a { padding: 14px 40px; border-radius: 10px; text-decoration: none;
        font-weight: bold; font-size: 1rem; }
    .btn-on  { background: #22c55e; color: #000; }
    .btn-off { background: #ef4444; color: #fff; }
    .btn-select { background: #0059ff; color: #fff; width: 90%; max-width: 300px;}
  </style>
</head>
<body>
  <h2>ESP32 - Car leds - Control Panel</h2>
  <p>Led Strip Mode: %STATUS_TEXT%</p>
  <a href="/ledStrip/mode/rpm1"  class="btn-select" >Mode 1</a>
  <a href="/ledStrip/mode/rpm2" class="btn-select">Mode 2</a>
  <a href="/ledStrip/mode/rpm3" class="btn-select">Mode 3</a>
  <a href="/ledStrip/mode/rpm4" class="btn-select">Mode 4</a>
  <a href="/ledStrip/mode/rpm5" class="btn-select">Mode 5</a>
  <p class="%BRIGHTNESS_CLASS%">Led Strip Brightness: %BRIGHTNESS_TEXT%</p>
  <a href="/ledStrip/light/255" class="btn-select">Day</a>
  <a href="/ledStrip/light/120" class="btn-select">Night</a>
  <a href="/update">Send code (upload and update)</a>
  <a href="/ledStrip/mode/test">Test Led Strip (All leds On)</a>

</body>
</html>
)rawliteral";